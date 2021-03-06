#include <stdlib.h>
#include <stdio.h>
#include <cStuff/dbx.h>
#include <cStuff/log.h>

#include "sql-queries.h"
#include "store.h"

/* -------------------------------------------------------------------------- */

status_t
store_media( channel_t            channel,
             arc_record_media_t   record,
             unsigned int         size )
{
  log_state(
    "MEDIA: %s (%s) of %ld bytes\n", record->code, record->type, record->size
  );

  FILE * f = fopen("/tmp/input.wav", "w");
  if (f)
  {
    fwrite(record->data, 1, record->size, f);
    fclose(f);
    log_state("/tmp/input.amr saved");
  }
  else
    log_error("/tmp/input.amr failed");

  

  return STATUS_SUCCESS;
}

/* -------------------------------------------------------------------------- */


