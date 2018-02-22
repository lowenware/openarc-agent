#include <stdlib.h>
#include <cStuff/dbx.h>

#include "sql-queries.h"
#include "store-heartbeat.h"

/* -------------------------------------------------------------------------- */

status_t
store_heartbeat(  channel_t                  channel,
                  arc_record_heartbeat_t     record,
                  size_t                     size )
{
  log_state("%s - %s\n", record->code, record->received);

  return STATUS_SUCCESS;
}

/* -------------------------------------------------------------------------- */


