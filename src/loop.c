/*
 * file loop.c - the center of agent's logic
 * here goes:
 *   - coordination of modules and channels
 *   - loading of channels from database
 *   - reloading of channels (not implemented yet)
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <cStuff/dbx.h>
#include <cStuff/log.h>
#include <cStuff/list.h>
#include <cStuff/str-utils.h>
#include <cStuff/sock-utils.h> /* u_sleep */

#include "status.h"
#include "daemon.h"
#include "configuration.h"
#include "channel.h"
#include "module.h"
#include "sql-queries.h"
#include "commands-pool.h"
#include "common-utils.h"
#include "store.h"
#include "loop.h"

/* -------------------------------------------------------------------------- */

#define MINIMAL_DELAY    100

#define INIT_ERROR_LOG_AND_GOTO( MOD, LABEL ) { \
    log_error( errModInit, MOD );               \
    goto LABEL;                                 \
  }

/* -------------------------------------------------------------------------- */

const char errModInit[ ] = "%s failed to initialize";
const char errFunction[] = "%s error";

/* -------------------------------------------------------------------------- */

static uint64_t  gChannelsIndex = 0;
static list_t    gChannels      = NULL;
static list_t    gModules       = NULL;


/* -------------------------------------------------------------------------- */

static status_t
load_module( module_t * module, const char * module_name )
{
  status_t   result;
  int        i;

  if (!module_name)
  {
    result = STATUS_NOT_FOUND;
    return result;
  }
  else
    result = STATUS_SUCCESS;

  for (i=0; i<gModules->count; i++)
  {
    *module = (module_t) list_index(gModules, i);
    if ( str_cmpi((*module)->name, module_name)==0 )
      return result;
  }

  if ( (result = module_new( module, module_name )) == STATUS_SUCCESS )
  {
    if (list_append( gModules, *module ) != -1)
    {
      log_state( "module '%s' loaded", module_name );
      return result;
    }
    else
      result = STATUS_MALLOC_ERROR;

    module_free( *module);
    *module = NULL;
  }

  return result;
}

/* -------------------------------------------------------------------------- */

static void
unload_module( module_t module )
{
  if ( module->refs )
  {
    log_debug(
        "unload_module(%s) called but module is referenced %d times; ignore",
        module->name,
        module->refs
    );
    return;
  }

  list_remove(gModules, module);
  module_free(module);

}

/* -------------------------------------------------------------------------- */

static void
remove_channel_by_id( uint32_t channel_id, bool not_unload_module )
{
  int       i;
  channel_t channel;
  module_t  module;

  for (i=0; i<gChannels->count; i++)
  {
    channel = (channel_t) list_index(gChannels, i);
    if (channel->id != channel_id) continue;

    if (channel->flags & CHANNEL_FLAG_READY)
      channel_close( channel );

    module = channel->module;

    channel_free( channel );
    list_remove_index(gChannels, i);

    if (!not_unload_module && !module->refs )
      unload_module( module );

    break;
  }
}

/* -------------------------------------------------------------------------- */

static void
remove_all_channels()
{
  int i;
  channel_t channel;

  while ( gChannels->count )
  {
    i = gChannels->count-1;

    channel = (channel_t) list_index(gChannels, i);

    if ( channel->flags & CHANNEL_FLAG_READY )
      channel_close( channel );

    channel_free( channel );

    list_remove_index( gChannels, i );
  }

  gChannelsIndex = 0;
}

/* -------------------------------------------------------------------------- */

static bool
load_channels_on_result( PGresult * res, int res_i, int * flags )
{
  uint32_t   ch_id;
  uint64_t   ch_index;
  char     * ch_name,
           * ch_uri,
           * ch_mod_name;

  int        i,
             count,
             ch_enabled;

  module_t   module;
  channel_t  channel;

  status_t   status;

  log_state( "loading channels" );

  count = PQntuples( res );

  for (i=0; i<count; i++)
  {
    ch_id       = dbx_as_integer( res, i, 0 );
    ch_index    = dbx_as_integer( res, i, 1 );
    ch_enabled  = dbx_as_bool(    res, i, 2 );
    ch_name     = dbx_as_string(  res, i, 3 );
    ch_uri      = dbx_as_string(  res, i, 4 );
    ch_mod_name = dbx_as_string(  res, i, 5 );

    status = load_module( &module, ch_mod_name );

    if ( status != STATUS_SUCCESS )
    {
      log_alert(
        "module loading failed; module=%s, channel=%s, status=%s",
        ch_mod_name,
        ch_name,
        u_status_to_text(status)
      );

      /* just ignore such channels, because modules could be missing */
      continue;
    }

    /* if channel exists, it will be closed and message will be logged,
     * otherwise nothing happens */
    remove_channel_by_id( ch_id, ch_enabled );

    /* add only enabled channels */
    if ( ch_enabled )
    {
      if ( !(channel = channel_new( ch_id, ch_name, ch_uri, module )) )
      {
        /* malloc error - exit function to retry
         * without LOAD_CHANNELS flag toggling */

        log_alert( "channel allocation failed" );
        goto finally;
      }

      if ( list_append(gChannels, channel) == -1 )
      {
        /* malloc error - exit function to retry
         * without LOAD_CHANNELS flag toggling */

        log_alert( "channel loading failed" );
        goto finally;
      }

      log_state("loaded channel %s", ch_name);
    }

    /* channels are sorted by index, so each next is higher that previous and
     * we don't need to compare ch_index with gChannelsIndex
     * also if next channel will fail for some reason, loading process will be
     * started respecting already loaded channels */
    gChannelsIndex = ch_index;
  }

  *flags ^= DAEMON_FLAG_LOAD_CHANNELS;

finally:
  *flags ^= DAEMON_FLAG_WAIT_CHANNELS;
  return true;
}

/* -------------------------------------------------------------------------- */

static void
load_channels_on_error( const char * e_message,
                        int          res_i,
                        int        * flags,
                        const char * e_sql )
{
  log_alert("failed to load channels, sql=(%s), error=(%s)", e_sql, e_message);

  *flags ^= DAEMON_FLAG_WAIT_CHANNELS;
}

/* -------------------------------------------------------------------------- */

static int
load_channels( int * flags )
{
  if ( *flags & DAEMON_FLAG_WAIT_CHANNELS )
    return MINIMAL_DELAY;

  uint64_t ret;

  ret = dbx_query_format(
          sqlSelectChannels,
          (dbx_on_result_t) load_channels_on_result,
          (dbx_on_error_t)  load_channels_on_error,
          flags,
          2, DBX_STRING, cfg->instance,
             DBX_UINT64, gChannelsIndex
        );

  /* zero retcode means malloc error,
   * in this case we will not set WAIT_CHANNELS
   * flag and attempt will be repeated */
  if (ret == 0)
    return MINIMAL_DELAY;

  *flags |= DAEMON_FLAG_WAIT_CHANNELS;
  return 0;
}

/* -------------------------------------------------------------------------- */

static int
touch_database(int * flags)
{
  const char c_error[] = "database error (%s)";
  int result = 0;

  switch ( dbx_touch() )
  {
    case STATUS_PENDING:
      result = MINIMAL_DELAY;

    case STATUS_SUCCESS:
      if ( !( *flags & DAEMON_FLAG_DATABASE_READY ))
      {
        if (dbx_ready_connections_count())
        {
          *flags |= DAEMON_FLAG_DATABASE_READY;

          if (*flags & DAEMON_FLAG_DATABASE_ERROR)
            *flags ^= DAEMON_FLAG_DATABASE_ERROR;

          log_state("database connected");
        }
      }
      return result;


    case STATUS_MALLOC_ERROR:
      if (! (*flags & DAEMON_FLAG_DATABASE_ERROR) )
        log_alert(c_error, u_status_to_text(STATUS_MALLOC_ERROR) );
      break;

    case STATUS_EXTCALL_ERROR:
      if (! (*flags & DAEMON_FLAG_DATABASE_ERROR) )
        log_alert(c_error, dbx_get_error() );
      break;
  }

  if ( ! dbx_ready_connections_count() )
  {
    if (*flags & DAEMON_FLAG_DATABASE_READY ) 
    {
      *flags ^= DAEMON_FLAG_DATABASE_READY;

      remove_all_channels();
    }

    *flags |= DAEMON_FLAG_DATABASE_ERROR;
  }


  /*
  if (do_reset)
    *flags ^= DAEMON_FLAG_DATABASE_ERROR;
    */
  return 5*MINIMAL_DELAY;
}

/* -------------------------------------------------------------------------- */

static int
touch_channel( channel_t channel )
{
  int             result = MINIMAL_DELAY;
  arc_command_t   command;
  arc_record_t  * list = NULL;
  unsigned int    size = 0;
  status_t        status = STATUS_PENDING;

  /* opening */
  if ( !( channel->flags & CHANNEL_FLAG_READY ) )
  {
    if (channel_open( channel ) != STATUS_SUCCESS)
      return result;

    result = 1;
  }

  /* input */
  if ((status = channel_recv(channel, &list, &size))==STATUS_SUCCESS)
  {
    if (size)
    {
      status = store_records(channel, list, size);

      if (status != STATUS_SUCCESS)
      {
        channel->module->confirm( channel->m_hd, list, 0 );
      }
    }

    result = 0;
  }

  void * cmd_data = NULL;

  /* output */
  if ( commands_pool_get(&command, channel, &cmd_data, &size) == STATUS_SUCCESS )
  {
    if ( channel_send( channel, command, cmd_data, size ) != STATUS_SUCCESS )
    {
      commands_pool_reset( command, channel, cmd_data );
    }
    result = 0;
  }

  return result;
}

/* -------------------------------------------------------------------------- */


/* -------------------------------------------------------------------------- */

status_t
loop_run( int * flags )
{
  status_t     result         = STATUS_MALLOC_ERROR;
  unsigned int ch_i           = 0;
  int          delay;

  /* init stage */
  result = dbx_init( cfg->dbUsername, 
                     cfg->dbPassword,
                     cfg->dbDatabase,
                     cfg->dbHostname,
                     cfg->dbPort,
                     cfg->dbConnections );

  if ( result != STATUS_SUCCESS )
    INIT_ERROR_LOG_AND_GOTO( "DBX", finally );

  if (! (gModules = list_new(4)) )
    INIT_ERROR_LOG_AND_GOTO( "modules list", release_dbx );

  if (! (gChannels = list_new(4)) )
    INIT_ERROR_LOG_AND_GOTO( "channels list", release_modules );

  if ( commands_pool_init() != STATUS_SUCCESS )
    INIT_ERROR_LOG_AND_GOTO( "commands-pool", release_channels );


  /* loop stage */

  result = STATUS_SUCCESS;

  log_state("agent started");

  while( ! (*flags & DAEMON_FLAG_TERMINATE) )
  {
    /* communicate with DB */
    delay = touch_database(flags);

    if ( *flags & DAEMON_FLAG_DATABASE_READY )
    {

      if (gChannels->count) /* communicate with channels */
      {
        delay = touch_channel( list_index(
                    gChannels, (ch_i < gChannels->count) ? (ch_i++):(ch_i = 0) 
                ) );
      }
      else 
        *flags |= DAEMON_FLAG_LOAD_CHANNELS;

      if ( *flags & DAEMON_FLAG_LOAD_CHANNELS )
        delay += load_channels( flags );
    }

    u_sleep( delay );

  }

  log_state("agent stopped");

  /* release stage */
  commands_pool_release();

release_channels:
  list_free(gChannels, (list_destructor_t) channel_free);
  gChannels = NULL;

release_modules:
  list_free(gModules, (list_destructor_t) module_free);
  gModules = NULL;

release_dbx:
 dbx_release();

finally:
  return result;
}

/* -------------------------------------------------------------------------- */
