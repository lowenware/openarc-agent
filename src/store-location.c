#include <stdlib.h>
#include <cStuff/dbx.h>

#include "sql-queries.h"
#include "store-location.h"

/* -------------------------------------------------------------------------- */

status_t
store_location( channel_t                 channel,
                arc_record_location_t     record,
                size_t                    size )
{

  printf(
    "%s - %s, (%Lf,%Lf)\n",
    record->code,
    record->received,
    record->latitude,
    record->longitude
  );

  return STATUS_SUCCESS;
}

/* -------------------------------------------------------------------------- */


