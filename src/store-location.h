#ifndef _AGENT_STORE_LOCATION_H_
#define _AGENT_STORE_LOCATION_H_

#include <openarc/module-sdk.h>

#include "status.h"
#include "channel.h"

/* -------------------------------------------------------------------------- */

status_t
store_location(   channel_t                  channel,
                  arc_record_location_t      record,
                  size_t                     size );

/* -------------------------------------------------------------------------- */

#endif
