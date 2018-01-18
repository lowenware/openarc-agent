#ifndef _AGENT_STORE_HEARTBEAT_H_
#define _AGENT_STORE_HEARTBEAT_H_

#include <openarc/module-sdk.h>

#include "status.h"
#include "channel.h"

/* -------------------------------------------------------------------------- */

status_t
store_heartbeat(  channel_t                  channel,
                  arc_record_heartbeat_t     record,
                  size_t                     size );

/* -------------------------------------------------------------------------- */

#endif
