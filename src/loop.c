#include <stdio.h>
#include <unistd.h>
#include <cStuff/dbx.h>
#include <cStuff/log.h>

#include "configuration.h"
#include "session.h"
#include "daemon.h"
#include "loop.h"

/* -------------------------------------------------------------------------- */

const char errModInit[ ] = "%s module failed to initialize";
const char errFunction[] = "%s error";

/* -------------------------------------------------------------------------- */

static status_t
loop_failure( const char * format, ... )
{
  va_list vl;

  va_start(vl, format);

  log_vprintf( &stdlog, LOG_LEVEL_ERROR, format, vl );

  va_end(vl);

  return STATUS_INIT_ERROR;
}

/* -------------------------------------------------------------------------- */

#define DB_READY 1
#define DB_RESET 2
#define DB_ERROR 4

static bool
database_check(int * flags)
{
  bool do_reset = (*flags) & DB_RESET,
       is_error = (*flags) & DB_ERROR;


  switch (dbx_touch( do_reset , is_error))
  {
    case DBX_SUCCESS:
      if ( !( *flags & DB_READY ))
      {
        *flags |= DB_READY;
        if (is_error) *flags ^= DB_ERROR;

        printf("database connected\n");
      }
      break;

    case DBX_CONNECTION_ERROR:
    case DBX_MALLOC_ERROR:
      if (! is_error )
      {
        fprintf(stderr, "datatbase connection error: %s\n", dbx_get_error());
        *flags |= DB_ERROR;
      }

      if (*flags & DB_READY ) *flags ^= DB_READY;

      *flags |= DB_RESET;
      break;

    default:
      break;
  }
  if (do_reset) *flags ^= DB_RESET;
  return true;
}

/* -------------------------------------------------------------------------- */

status_t
loop_run( int * flags )
{
  status_t result = STATUS_SUCCESS;

  if ( (result = dbx_init()) != STATUS_SUCCESS )
    goto finally;

  while( ! (*flags & DAEMON_TERMINATE) )
  {
    
  }

 dbx_release();

finally:
  return result;
}

/* -------------------------------------------------------------------------- */
