#include <stdlib.h>
#include <cStuff/dbx.h>

#include "sql-queries.h"
#include "store-media.h"

/* -------------------------------------------------------------------------- */

status_t
store_media( channel_t            channel,
             arc_record_media_t   record,
             size_t               size )
{
  printf(
    "talkback %s (%s) of %ld bytes\n", record->code, record->type, record->size
  );

  return STATUS_SUCCESS;
}

/* -------------------------------------------------------------------------- */


