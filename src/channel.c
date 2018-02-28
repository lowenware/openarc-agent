#include <stdio.h>

#include <stdlib.h>
#include <string.h>

#include <cStuff/log.h>
#include <cStuff/uri.h>
#include <cStuff/list.h>
#include <cStuff/str-utils.h>
#include <cStuff/query-stream.h>

#include "common-utils.h"
#include "channel.h"

/* -------------------------------------------------------------------------- */

static const char cBadReply[] = "[%s] bad module reply; code=%d";
static const char cProtocol[] = ARC_MODULE_PROTOCOL;
static const char cUsername[] = ARC_MODULE_USERNAME;
static const char cPassword[] = ARC_MODULE_PASSWORD;
static const char cAddress[]  = ARC_MODULE_ADDRESS;
static const char cPort[]     = ARC_MODULE_PORT;
static const char cPath[]     = ARC_MODULE_PATH;

/* -------------------------------------------------------------------------- */

#define ADD_PAIR( KEY, VAL ) ( !(       \
  ( uri->VAL > 0 ) && (                 \
  result = _add_key_value(              \
                *params,                \
               KEY,                    \
                sizeof(KEY)-1,          \
                &uri_str[uri->VAL],     \
                uri->VAL##_len          \
              ) ) != CSTUFF_SUCCESS ) )

/* -------------------------------------------------------------------------- */

static status_t
_add_key_value( list_t       params,
                const char * key,
                int          key_len,
                const char * value,
                int          value_len )
{
  status_t result = STATUS_SUCCESS;

  int    i=0,
         j=0;
  char * pair = calloc( key_len+1+value_len+1, sizeof(char) );

  if (pair)
  {
    for (i=0; i<key_len; i++)
    {
      pair[j++]=key[i];
    }

    pair[j++]='=';

    for (i=0; i<value_len; i++)
    {
      pair[j++]=value[i];
    }

    if ( list_append( params, pair ) != -1 )
      return result;

    free(pair);
  }

  result = STATUS_MALLOC_ERROR;

  return result;
}

/* -------------------------------------------------------------------------- */

static status_t
_uri_to_params_list( const char * uri_str, list_t * params )
{
  status_t         result = STATUS_SUCCESS;
  uri_t            uri;
  query_stream_t   qs;
  const char     * ptr;


  if ( !(uri = calloc(1, sizeof(struct uri))) )
    return STATUS_MALLOC_ERROR;


  if ( !(*params = list_new(4)) ) 
  {
    result = STATUS_MALLOC_ERROR;
    goto except;
  }

  if ( !(uri_parse(uri_str, strlen(uri_str), uri)) )
  {
    result = STATUS_EXTCALL_ERROR;
    goto finally;
  }

  result  = _add_key_value(
              *params, cProtocol, sizeof(cProtocol)-1,
                       uri_str,   uri->protocol_len
            );

  if (result != CSTUFF_SUCCESS) goto except;

  if ( ! ADD_PAIR( cUsername, user ) )
    goto except;

  if ( ! ADD_PAIR( cPassword, password ) )
    goto except;

  if ( ! ADD_PAIR( cAddress, address ) )
    goto except;

  if ( ! ADD_PAIR( cPort, port ) )
    goto except;

  if ( ! ADD_PAIR( cPath, path ) )
    goto except;

  /* query */

  if ( (uri->query > 0) && (uri->query_len > 0) )
  { 
    if ( ! (qs=query_stream_new('&')) )
    {
      result = STATUS_MALLOC_ERROR;
      goto except;
    }

    ptr = &uri_str[uri->query];

    while( query_stream_read(qs, ptr, uri->query_len, true) )
    {
      result = _add_key_value( 
                  *params, qs->key,   qs->key_len,
                           qs->value, qs->value_len
               );

      if (result != STATUS_SUCCESS)
        break;
    }

    query_stream_free(qs);

    if ( result == STATUS_SUCCESS )
      goto finally;
  }

  goto finally;

except:
  list_free( *params, free );
  *params = NULL;

finally:
  free(uri);

  return result;
}

/* -------------------------------------------------------------------------- */

channel_t
channel_new( uint32_t     id, 
             const char * name,
             const char * uri,
             module_t     module )
{
  channel_t      self;
  arc_status_t   status;
  list_t         conf=NULL;

  if( (self = calloc(1, sizeof(struct channel))) != NULL )
  {
    self->id=id;
    self->module = module;
    module->refs++;
printf( "channel: structure allocated\n");

    if ( (self->name = str_copy(name)) != NULL )
    {
printf( "channel: name allocated\n");
      /* get channel handler from driver */
      status = module->alloc_handle( (void *) &self->m_hd);

      if ( status == ARC_STATUS_SUCCESS )
      {
printf( "channel: handle allocated\n");
        /* prepare parameters */
        if ( _uri_to_params_list(uri, &conf) == STATUS_SUCCESS)
        {
printf( "channel: params allocated\n");
          /* set up channel */
          status = module->set_handle(
                     self->m_hd,
                     conf->count,
                     (char**) conf->list
                   );

printf( "channel: set handle returned %d\n", status);
          list_free(conf, free);

          if (status == ARC_STATUS_SUCCESS)
{
printf( "channel: config allocated\n");
            return self;
}
        }
      }

    }

    channel_free(self);
    self = NULL;
  }

  return self;
}

/* -------------------------------------------------------------------------- */

void
channel_free( channel_t self )
{
  self->module->refs--;

  if (self->name)
    free(self->name);

  if (self->m_hd)
    self->module->free_handle( self->m_hd );

  free(self);
}


/* -------------------------------------------------------------------------- */

status_t
channel_open( channel_t self )
{
  arc_status_t   status;
  const char * ch_ptr;

  status = self->module->open(self->m_hd);

  switch (status)
  {
    case ARC_STATUS_SUCCESS:
      self->flags |= CHANNEL_FLAG_READY;

      if (self->flags & CHANNEL_FLAG_ERROR)
        self->flags ^= CHANNEL_FLAG_ERROR;

      log_state( "[%s] channel open", self->name);
      return STATUS_SUCCESS;

    case ARC_STATUS_IDLE:
      return STATUS_PENDING;

    case ARC_STATUS_ERROR:
      self->flags |= CHANNEL_FLAG_ERROR;

      status = self->module->get_error(self->m_hd, &ch_ptr);

      if (status != ARC_STATUS_SUCCESS)
        ch_ptr = u_status_to_text( STATUS_EXTCALL_ERROR );

      log_alert( "[%s] channel opening failed; error=%s", self->name, ch_ptr );

      return STATUS_EXTCALL_ERROR;

    default:
      log_alert( cBadReply, self->name, status );
      return STATUS_EXTCALL_ERROR;

  }
}

/* -------------------------------------------------------------------------- */

status_t
channel_close( channel_t self )
{
  arc_status_t   status;
  const char * ch_ptr;

  status = self->module->close(self->m_hd);

  switch (status)
  {
    case ARC_STATUS_SUCCESS:
      if (self->flags & CHANNEL_FLAG_READY)
        self->flags ^= CHANNEL_FLAG_READY;

      log_state( "[%s] channel closed", self->name);
      return STATUS_SUCCESS;

    case ARC_STATUS_IDLE:
      return STATUS_PENDING;

    case ARC_STATUS_ERROR:
      self->flags |= CHANNEL_FLAG_ERROR;

      status = self->module->get_error( self->m_hd, &ch_ptr );

      if (status != ARC_STATUS_SUCCESS)
        ch_ptr = u_status_to_text( STATUS_EXTCALL_ERROR );

      log_alert( "[%s] channel closing failed; error=%s", self->name, ch_ptr );

      return STATUS_EXTCALL_ERROR;

    default:
      log_alert( cBadReply, self->name, status );
      return STATUS_EXTCALL_ERROR;

  }
}

/* -------------------------------------------------------------------------- */

status_t
channel_recv( channel_t       self,
              arc_record_t *  record,
              unsigned int *  count,
              void         ** data)
{
  arc_status_t status;
  const char * ch_ptr;

  status = self->module->read( self->m_hd, record, count, data );

  switch(status)
  {
    case ARC_STATUS_SUCCESS:
      log_state(
        "[%s] --> %u x %s", self->name, *count, u_arc_record_to_text(*record)
      );
      return STATUS_SUCCESS;

    case ARC_STATUS_IDLE:
      return STATUS_PENDING;

    case ARC_STATUS_ERROR:
      self->flags |= CHANNEL_FLAG_ERROR;

      status = self->module->get_error( self->m_hd, &ch_ptr);

      if (status != ARC_STATUS_SUCCESS)
        ch_ptr = u_status_to_text( STATUS_EXTCALL_ERROR );

      log_alert( "[%s] channel recv() failed; error=%s", self->name, ch_ptr );

    default:
      log_alert( cBadReply, self->name, status );
      return STATUS_EXTCALL_ERROR;

  }

}

/* -------------------------------------------------------------------------- */

status_t
channel_send( channel_t       self,
              arc_command_t   command,
              unsigned int    count,
              void          * data )
{
  arc_status_t   status;
  const char   * ch_ptr;

  status = self->module->write(self->m_hd, command, count, data);

  switch (status)
  {
    case ARC_STATUS_SUCCESS:
      log_state("[%s] <-- %s", self->name, u_arc_command_to_text(command));
      return STATUS_SUCCESS;

    case ARC_STATUS_IDLE:
      return STATUS_PENDING;

    case ARC_STATUS_ERROR:
      self->flags |= CHANNEL_FLAG_ERROR;

      status = self->module->get_error( self->m_hd, &ch_ptr );

      if (status != ARC_STATUS_SUCCESS)
        ch_ptr = u_status_to_text( STATUS_EXTCALL_ERROR );

      log_alert(
        "[%s] channel send(%s) failed; error=%s",
        self->name,
        u_arc_command_to_text(command),
        ch_ptr
      );

      return STATUS_EXTCALL_ERROR;

    default:
      log_alert( cBadReply, self->name, status );
      return STATUS_EXTCALL_ERROR;

  }

}

/* -------------------------------------------------------------------------- */

