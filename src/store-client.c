#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>

#include <cStuff/dbx.h>

#include "sql-queries.h"
#include "store-client.h"

/* -------------------------------------------------------------------------- */

status_t
store_client(  channel_t               channel,
               arc_record_t            record_type,
               arc_record_client_t     record,
               size_t                  size )
{
  char host[INET_ADDRSTRLEN+1];
  int  port;

  inet_ntop( 
    AF_INET, &record->address.sin_addr, host, INET_ADDRSTRLEN
  );

  port = (int)(ntohs( record->address.sin_port )) & 0xFFFF;

  printf(
    "client %s:%d(%"PRIu64") - %s\n",
    host,
    port,
    record->callsign,
    ((record_type==ARC_RECORD_CLIENT_CONNECTED) ? "connected" : "disconnected")
  );

  return STATUS_SUCCESS;
}

/* -------------------------------------------------------------------------- */


