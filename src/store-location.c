#include <stdlib.h>
#include <cStuff/dbx.h>
#include <cStuff/log.h>
#include <cStuff/str-utils.h>

#include "sql-queries.h"
#include "store.h"

status_t
store_location( channel_t                 channel,
                arc_record_location_t     record,
                unsigned int              size )
{
  char * recd_sql = NULL,
       * list_sql = NULL,
       * sql      = NULL;

  arc_record_location_t r = record;
  store_t store;

  uint64_t dbx_id;
  unsigned int sqls_count = size * SQLS_COUNT + 2;

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

  if ( !(store = store_new(channel, record, ARC_RECORD_LOCATION, sqls_count)) )
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
