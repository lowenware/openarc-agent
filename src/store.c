#include <stdlib.h>
#include <locale.h>
#include <cStuff/log.h>
#include <cStuff/base64.h>
#include <cStuff/str-utils.h>
#include "store.h"
#include "common-utils.h"

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
  void          * list;
  unsigned int    list_size;
  unsigned int    sqls_count;

};

typedef struct store * store_t;

/* -------------------------------------------------------------------------- */

static store_t
store_new( channel_t       channel,
           arc_record_t  * list,
           unsigned int    list_size,
           unsigned int    sqls_count )
{
  store_t self;

  if ( (self = malloc(sizeof(struct store))) != NULL )
  {
    self->channel      = channel;
    self->list         = list;
    self->list_size    = list_size;
    self->sqls_count   = sqls_count;
  }

  return self;
}

/* -------------------------------------------------------------------------- */

#define store_free(self) free(self);

/* -------------------------------------------------------------------------- */

static bool
store_on_sql_result( PGresult * res, int i_res, store_t store )
{
  channel_t channel = store->channel;

  printf("STORE RESULT: %d of %u\n", i_res, store->sqls_count);

  if (i_res+1 != store->sqls_count)
    return true;

  store->channel->module->confirm( channel->m_hd, store->list, 1 );

  log_state("[%s] stored %u records", channel->name, store->list_size);

  store_free(store);

  return false;
}

/* -------------------------------------------------------------------------- */

static void
store_on_sql_error( const char * e_msg,
                    int          i_res,
                    store_t      store,
                    const char * sql )
{
  channel_t channel = store->channel;

  printf("STORE ERROR: %d of %u\n", i_res, store->sqls_count);

  channel->module->confirm( channel->m_hd, store->list, 0 );

  log_error("[%s] not stored %u records (%s)", channel->name,
                                      store->list_size,
                                      e_msg);
  log_debug("[SQL] %s", sql);
  store_free(store);
}


/* -------------------------------------------------------------------------- */

static const char sqlHeartBeat[] =
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

#define HEARTBEAT_SQLS 2

/* -------------------------------------------------------------------------- */

static char *
get_heartbeat_sql( channel_t channel, arc_record_heartbeat_t record)
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

static const char sqlLocation[] =
    /* -- analyse signal, save result into records table */
    "INSERT INTO records ("
      "received_at,"
      "occured_at,"
      "event_id,"
      "source_type_id,"
      "source_id,"
      "source_label,"
      "channel_id,"
      "device_id,"
      "callsign,"
      "event_code,"
      "gsm_level,"
      "bat_level,"
      "position,"
      "altitude,"
      "speed,"
      "direction,"
      "message"
    ") "
    "SELECT "
      "$4," /* received_at */
      "CONCAT($5, ' ', tz.name)::TIMESTAMP WITH TIME ZONE," /* occured_at */
      "CASE WHEN p.event_id IS NULL THEN ("
        "SELECT e.id event_id "
        "FROM events e JOIN patterns p on p.event_id=e.id "
        "WHERE p.dictionary_id=1 AND p.event_code='UNKNOWN'"
      ") ELSE p.event_id END,"
      "1," /* source_type_id = 1 (agent)*/
      "a.id,"
      "a.name,"
      "dc.channel_id,"
      "dc.device_id,"
      "dc.callsign,"
      "CASE WHEN p.event_id IS NULL THEN (SELECT 'UNKNOWN'::varchar) "
      "ELSE p.event_code END,"
      "$6," /* gsm_level */
      "$7," /* bat_level */
      "$8," /* latitude, longitude */
      "$9," /* altitude */
      "$10," /* speed */
      "$11,"  /* direction */
      "CASE WHEN p.event_id IS NULL THEN $3 ELSE NULL END "
    "FROM "
      "device_channels dc INNER JOIN "
      "devices d ON dc.device_id=d.id INNER JOIN "
      "timezones tz ON d.timezone_id=tz.id INNER JOIN "
      "channels c ON dc.channel_id=c.id INNER JOIN "
      "agents a ON c.agent_id=a.id LEFT JOIN "
      "dictionaries dct ON dc.dictionary_id=dct.id LEFT JOIN "
      "patterns p ON p.dictionary_id=dct.id AND p.event_code=$3 LEFT JOIN "
      "events e ON p.event_id=e.id "
    "WHERE "
      "dc.callsign=$2 AND "  /* callsign */
      "dc.channel_id=$1 AND " /* channel_id */
      "d.enabled=TRUE;\n"

    /* update device information */
    "UPDATE "
      "devices d "
    "SET "
      "$12"
      "updated_at=(now() at time zone 'utc'),"
      "is_alarm=(CASE WHEN e.type_id=100 THEN TRUE ELSE is_alarm END),"
      "is_fault=(CASE WHEN e.type_id=200 THEN TRUE ELSE is_fault END),"
      "is_online=TRUE "
    "FROM "
      "records r, events e "
    "WHERE "
      "r.id=currval('records_id_seq') AND "
      "r.event_id=e.id AND "
      "d.id=r.device_id AND "
      "d.enabled=TRUE;\n"

    /* update device_channels heartbeat */
    "UPDATE "
      "device_channels dc "
    "SET "
      "heartbeat_at=$4," /* recieved_at */
      "is_online=TRUE "
    "FROM "
      "records r, devices d "
    "WHERE "
      "r.id=currval('records_id_seq') AND "
      "dc.channel_id=r.channel_id AND "
      "dc.callsign=r.callsign AND "
      "d.id=dc.device_id AND "
      "d.enabled=TRUE;\n"

    /* select data for further manipulations */
    "SELECT "
      "r.id record_id,"
      "r.event_id,"
      "e.type_id event_type_id,"
      "r.device_id "
    "FROM "
      "records r,"
      "events e "
    "WHERE "
      "r.id=currval('records_id_seq') AND "
      "r.event_id=e.id;\n";

#define LOCATION_SQLS 4

/* -------------------------------------------------------------------------- */

static char *
get_location_sql( channel_t channel, arc_record_location_t   record)
{
  const char sqlSetFormat[] = "%s='%s',";

  char * ptr,
       * update_set = NULL,
       * gsm_level  = NULL,
       * bat_level  = NULL,
       * position   = NULL,
       * altitude   = NULL,
       * speed      = NULL,
       * direction  = NULL,
       * result     = NULL;

  if (!(record->gsmLevel < 0 || record->gsmLevel > 100))
  {
    gsm_level = str_printf(GSM_LEVEL_FORMAT, record->gsmLevel & 0xFF);

    if (!gsm_level)
      goto finally;

    ptr = str_printf( sqlSetFormat, "gsm_level", gsm_level );
    if ( !ptr )
      goto finally;

    result = str_cat(update_set, ptr);
    free(ptr);
    if (!result)
      goto finally;

    update_set = result;
  }

  if (!(record->batLevel < 0 || record->batLevel > 100))
  {
    bat_level = str_printf(BAT_LEVEL_FORMAT, record->batLevel & 0xFF);

    if (!bat_level)
      goto finally;

    ptr = str_printf( sqlSetFormat, "bat_level", bat_level );
    if ( !ptr )
      goto finally;

    result = str_cat(update_set, ptr);
    free(ptr);
    if (!result)
      goto finally;

    update_set = result;
  }

  if ( !( record->latitude < -90.0   || record->latitude > 90.0 ||
          record->longitude < -180.0 || record->longitude > 180.0 ) )
  {
    position = str_printf(POSITION_FORMAT, record->latitude, record->longitude);
    if (!position)
      goto finally;

    ptr = str_printf( "position=%s,", position );
    if ( !ptr )
      goto finally;

    result = str_cat(update_set, ptr);
    free(ptr);
    if (!result)
      goto finally;

    update_set = result;
  }

  if  ( !( record->altitude<0) )
  {
    altitude = str_printf( ALTITUDE_FORMAT, record->altitude );
    if (!altitude)
      goto finally;

    ptr = str_printf( sqlSetFormat, "altitude", altitude );
    if ( !ptr )
      goto finally;

    result = str_cat(update_set, ptr);
    free(ptr);
    if (!result)
      goto finally;

    update_set = result;
  }

  if ( !(record->speed<0) )
  {
    speed = str_printf( SPEED_FORMAT, record->speed );
    if (!speed)
      goto finally;

    ptr = str_printf( sqlSetFormat, "speed", speed );
    if (!ptr)
      goto finally;

    result = str_cat(update_set, ptr);
    free(ptr);
    if (!result)
      goto finally;

    update_set = result;
  }

  if ( !(record->direction<0) )
  {
    direction = str_printf( DIRECTION_FORMAT, record->direction );
    if (!direction)
      goto finally;

    ptr = str_printf( sqlSetFormat, "direction", direction );
    if (!ptr)
      goto finally;

    result = str_cat(update_set, ptr);
    free(ptr);
    if (!result)
      goto finally;

    update_set = result;
  }

  result = dbx_sql_format(
             sqlLocation, 12,
             DBX_UINT32, channel->id,
             DBX_UINT64, record->callsign,
             DBX_STRING, record->code,
             DBX_STRING, record->received,
             DBX_STRING, record->occured,
             DBX_STRING, gsm_level,
             DBX_STRING, bat_level,
             DBX_STATEMENT, position,
             DBX_STRING, altitude,
             DBX_STRING, speed,
             DBX_STRING, direction,
             DBX_STATEMENT, (update_set ? update_set : "")
           );

finally:
  if (gsm_level)
    free(gsm_level);

  if (bat_level)
    free(bat_level);

  if (position)
    free(position);

  if (altitude)
    free(altitude);

  if (speed)
    free(speed);

  if (direction)
    free(direction);

  if (update_set)
    free(update_set);

  return result;
}


/* -------------------------------------------------------------------------- */

static const char sqlMedia[] =
  "WITH insert_media AS ("
    "INSERT INTO medias (device_id, received_at, mime_type, size, data) "
    "SELECT "
      "dc.device_id,"
      "$3,"
      "$4,"
      "$5,"
      "$6 "
    "FROM "
      "device_channels dc JOIN "
      "devices d ON dc.device_id=d.id "
    "WHERE "
      "dc.channel_id=$1 AND "
      "dc.callsign=$2 AND "
      "d.enabled=TRUE "
    "RETURNING id"
  ")"
  "INSERT INTO records ("
      "received_at,"
      "occured_at,"
      "event_id,"
      "source_type_id,"
      "source_id,"
      "source_label,"
      "channel_id,"
      "device_id,"
      "callsign,"
      "event_code,"
      "message"
    ") "
    "SELECT "
      "$3," /* received_at */
      "$3," /* occured_at */
      "p.event_id,"
      "1," /* source_type_id = 1 (agent)*/
      "a.id,"
      "a.name,"
      "$1,"
      "dc.device_id,"
      "$2,"
      "p.event_code,"
      "(SELECT id FROM insert_media)"
    "FROM "
      "device_channels dc INNER JOIN "
      "devices d ON dc.device_id=d.id INNER JOIN "
      "channels c ON dc.channel_id=c.id INNER JOIN "
      "agents a ON c.agent_id=a.id LEFT JOIN "
      "dictionaries dct ON dc.dictionary_id=dct.id LEFT JOIN "
      "patterns p ON p.dictionary_id=dct.id AND p.event_code='TALKBACK' LEFT JOIN "
      "events e ON p.event_id=e.id "
    "WHERE "
      "dc.callsign=$2 AND "  /* callsign */
      "dc.channel_id=$1 AND " /* channel_id */
      "d.enabled=TRUE;\n";


#define MEDIA_SQLS 1

static char *
get_media_sql( channel_t channel, arc_record_media_t   record)
{
  char * result;
  unsigned char *raw_data = (unsigned char *) record->data;
  unsigned char *b64_data = NULL;
  int o_len = 0;

  if ( base64_encode(raw_data, record->size, &b64_data, &o_len) == 0 )
  {

    result = dbx_sql_format(
               sqlMedia, 6,
               DBX_UINT32, channel->id,
               DBX_UINT64, record->callsign,
               DBX_STRING, record->received,
               DBX_STRING, record->type,
               DBX_UINT32, o_len,
               DBX_CONSTANT, b64_data
             );

    free(b64_data);

  }
  else
    result = NULL;

  return result;
}

/* -------------------------------------------------------------------------- */


status_t
store_records( channel_t channel, arc_record_t * list, unsigned int size)
{
  char * recd_sql = NULL,
       * list_sql = NULL,
       * sql      = NULL;

  store_t store;

  uint64_t dbx_id;
  unsigned int sqls_count = 2;

printf("store %u records (%p)\n", size, (void*)list);

  setlocale(LC_ALL, "C");

  for (unsigned int i=0; i<size; i++)
  {
    arc_record_t r = list[i];

printf("store switch(%p)\n", (void*)r);
    switch (r->recordClass)
    {
      case ARC_RECORD_HEARTBEAT:
printf("store ARC_RECORD_HEARTBEAT\n");
        recd_sql = get_heartbeat_sql(
                     channel, (arc_record_heartbeat_t) r
                   );
        sqls_count += HEARTBEAT_SQLS;
        break;

      case ARC_RECORD_CLIENT_CONNECTED:
      case ARC_RECORD_CLIENT_DISCONNECTED:
      case ARC_RECORD_CONTACT_ID:
printf("store %s\n",u_arc_record_to_text(r->recordClass));
        continue;
      /*
        recd_sql = get_client_connected_sql(
                     channel, (arc_record_client_t) r
                   );
        break;

      */
      /*
        recd_sql = get_client_disconnected_sql(
                     channel, (arc_record_client_t) r
                   );
        break;
        recd_sql = get_contact_id_sql(
                     channel, (arc_record_contact_id_t) r
                   );
        break;
      */

      case ARC_RECORD_LOCATION:
printf("store ARC_RECORD_LOCATION\n");
        recd_sql = get_location_sql(
                     channel, (arc_record_location_t) r
                   );
        sqls_count += LOCATION_SQLS;
        break;

      case ARC_RECORD_MEDIA:
printf("store ARC_RECORD_MEDIA\n");
        recd_sql = get_media_sql(
                     channel, (arc_record_media_t) r
                   );
        sqls_count += MEDIA_SQLS;
        break;

      default:
printf("store DEFAULT\n");
        log_alert(
          "[%s] unexpected record %d(%s)",
          channel->name,
          r->recordClass,
          u_arc_record_to_text(r->recordClass)
        );
        continue;
    }

    if (!recd_sql) goto except;

    if (!(sql = str_cat(list_sql, recd_sql)) )
      goto except;

printf("store 1\n");
    list_sql = sql;

    free(recd_sql);
    recd_sql = NULL;

  }

  setlocale(LC_ALL, "");

  if (!list_sql)
  {
    return STATUS_SUCCESS; /* unhandled */
  }

  if ( !(store = store_new(channel, list, size, sqls_count)) )
    goto except;

printf("SQL>\n%s\n", list_sql);

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
