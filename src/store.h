#ifndef _AGENT_STORE_H_
#define _AGENT_STORE_H_

#include <stdbool.h>
#include <openarc/module-sdk.h>
#include <sys/types.h>
#include <cStuff/dbx.h>
#include "channel.h"
#include "status.h"

/* -------------------------------------------------------------------------- */

#define POSITION_FORMAT  "POINT(%2.9Lf,%3.9Lf)"
#define BAT_LEVEL_FORMAT "%3d"
#define GSM_LEVEL_FORMAT "%3d"
#define ALTITUDE_FORMAT  "%6.2f"
#define SPEED_FORMAT     "%6.2f"
#define DIRECTION_FORMAT "%6.2f"

/* -------------------------------------------------------------------------- */

struct store
{
  channel_t       channel;
  void          * records;
  size_t          sqls_count;
  arc_record_t    records_type;
};

typedef struct store * store_t;

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

store_t
store_new( channel_t       channel,
           void          * records,
           arc_record_t    records_type,
           size_t          sqls_count );

#define store_free(self) free(self);

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

bool
store_on_sql_result( PGresult * res, int i_res, store_t store );

/* -------------------------------------------------------------------------- */

void
store_on_sql_error( const char * e_msg,
                    int          i_res,
                    store_t      store,
                    const char * sql );


/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */


status_t
store_client(  channel_t               channel,
               arc_record_t            record_type,
               arc_record_client_t     record,
               size_t                  size );

/* -------------------------------------------------------------------------- */

status_t
store_contact_id( channel_t                  channel,
                  arc_record_contact_id_t    record,
                  size_t                     size );

/* -------------------------------------------------------------------------- */

status_t
store_heartbeat(  channel_t                  channel,
                  arc_record_heartbeat_t     record,
                  size_t                     size );

/* -------------------------------------------------------------------------- */

status_t
store_location(   channel_t                  channel,
                  arc_record_location_t      record,
                  size_t                     size );

/* -------------------------------------------------------------------------- */

status_t
store_media( channel_t             channel,
             arc_record_media_t    record,
             size_t                size );

/* -------------------------------------------------------------------------- */

#endif
