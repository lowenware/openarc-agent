#include <stdlib.h>
#include <cStuff/log.h>
#include "store.h"
#include "common-utils.h"
/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

store_t
store_new( channel_t       channel,
           void          * records,
           arc_record_t    records_type,
           unsigned int    sqls_count)
{
  store_t self;

  if ( (self = malloc(sizeof(struct store))) != NULL )
  {
    self->channel      = channel;
    self->records      = records;
    self->records_type = records_type;
    self->sqls_count   = sqls_count;
  }

  return self;
}

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

bool
store_on_sql_result( PGresult * res, int i_res, store_t store )
{
  channel_t channel = store->channel;

  printf("STORE RESULT: %d of %u\n", i_res, store->sqls_count);

  if (i_res != store->sqls_count)
    return true;

  store->channel->module->confirm_record( channel->m_hd, store->records, 1 );

  log_state("[%s] %s stored", channel->name, 
                              u_arc_record_to_text(store->records_type));

  store_free(store);

  return false;
}

/* -------------------------------------------------------------------------- */

void
store_on_sql_error( const char * e_msg,
                    int          i_res,
                    store_t      store,
                    const char * sql )
{
  channel_t channel = store->channel;

  printf("STORE ERROR: %d of %u\n", i_res, store->sqls_count);

  channel->module->confirm_record( channel->m_hd, store->records, 0 );

  log_error("[%s] %s not stored: %s", channel->name, 
                                      u_arc_record_to_text(store->records_type),
                                      e_msg);
  log_debug("[SQL] %s", sql);
  store_free(store);
}


/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

