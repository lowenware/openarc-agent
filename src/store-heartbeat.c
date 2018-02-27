#include <stdlib.h>
#include <cStuff/str-utils.h>

#include "sql-queries.h"
#include "store.h"

const char sqlHeartBeat[] =
    /* update device information */
    "UPDATE "
      "devices d "
    "SET "
      "updated_at=(now() at time zone 'utc'), "
      "$1 " /* gsm_level */
      "$2 " /* bat_level */
      "is_online=TRUE "
    "FROM "
      "device_channels dc "
    "WHERE "
      "d.id=dc.device_id AND "
      "dc.callsign=$3 AND "
      "dc.channel_id=$4 AND "
      "d.enabled=TRUE;\n"

    /* update device_channels heartbeat */
    "UPDATE "
      "device_channels dc "
    "SET "
      "heartbeat_at=$5, " /* recieved_at */
      "is_online=TRUE "
    "FROM "
      "devices d "
    "WHERE "
      "d.id=dc.device_id AND "
      "dc.callsign=$3 AND "
      "dc.channel_id=$4 AND "
      "d.enabled=TRUE;\n";

#define SQLS_COUNT 2
/* -------------------------------------------------------------------------- */

static char *
get_single_sql( channel_t channel, arc_record_heartbeat_t record )
{
  char * gsm_level = NULL,
       * bat_level = NULL,
       * result    = NULL;
  
  if (!(record->gsmLevel < 0 || record->gsmLevel > 100))
  {
    gsm_level = str_printf(
                  "gsm_level="GSM_LEVEL_FORMAT",", record->gsmLevel & 0xFF
                );

    if (!gsm_level)
      goto finally;
  }

  if (!(record->batLevel < 0 || record->batLevel > 100))
  {
    bat_level = str_printf(
                  "bat_level="BAT_LEVEL_FORMAT",", record->batLevel & 0xFF
                );

    if (!bat_level)
      goto finally;
  }

  result = dbx_sql_format(
             sqlHeartBeat, 5,
             DBX_STATEMENT, (gsm_level ? gsm_level : ""),
             DBX_STATEMENT, (bat_level ? bat_level : ""),
             DBX_UINT64, record->callsign,
             DBX_UINT32, channel->id,
             DBX_STRING, record->received
           );

finally:
  if (gsm_level)
    free(gsm_level);

  if (bat_level)
    free(bat_level);

  return result;
}

/* -------------------------------------------------------------------------- */

status_t
store_heartbeat(  channel_t                  channel,
                  arc_record_heartbeat_t     record,
                  unsigned int               size )
{
  char * recd_sql = NULL,
       * list_sql = NULL,
       * sql      = NULL;

  arc_record_heartbeat_t r = record;
  store_t store;

  uint64_t dbx_id;

  while (size--)
  {
    if ( !(recd_sql = get_single_sql(channel, r)) )
      goto except;

    if ( !(sql = str_cat(list_sql, recd_sql)) )
      goto except;

    list_sql = sql;

    free(recd_sql);
    recd_sql = NULL;

    r++;
  }

  if ( !(store = store_new( channel, record, ARC_RECORD_HEARTBEAT, SQLS_COUNT * size+2 )) )
    goto except;

  dbx_id = dbx_query_transaction(
             list_sql,
             (dbx_on_result_t) store_on_sql_result,
             (dbx_on_error_t)  store_on_sql_error,
             store
           );

  free(list_sql);

  if (dbx_id == 0)
  {
    store_free(store);
    return STATUS_DATABASE_ERROR;
  }
  else
    return STATUS_SUCCESS;

except:
  if (recd_sql)
    free(recd_sql);

  if(list_sql)
    free(list_sql);

  return STATUS_MALLOC_ERROR;

}




/* -------------------------------------------------------------------------- */


