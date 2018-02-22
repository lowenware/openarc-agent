#include <stdlib.h>
#include <cStuff/dbx.h>
#include <cStuff/log.h>

#include "sql-queries.h"
#include "store-media.h"

/* -------------------------------------------------------------------------- */

status_t
store_media( channel_t            channel,
             arc_record_media_t   record,
             size_t               size )
{
  log_state(
    "MEDIA: %s (%s) of %ld bytes\n", record->code, record->type, record->size
  );

  return STATUS_SUCCESS;
}

/* -------------------------------------------------------------------------- */


