#ifndef _AGENT_STORE_CLIENT_H_
#define _AGENT_STORE_CLIENT_H_

#include <openarc/module-sdk.h>

#include "status.h"
#include "channel.h"

/* -------------------------------------------------------------------------- */

status_t
store_client(  channel_t               channel,
               arc_record_t            record_type,
               arc_record_client_t     record,
               size_t                  size );

/* -------------------------------------------------------------------------- */

#endif
