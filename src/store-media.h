#ifndef _AGENT_STORE_MEDIA_H_
#define _AGENT_STORE_MEDIA_H_

#include <openarc/module-sdk.h>

#include "status.h"
#include "channel.h"

/* -------------------------------------------------------------------------- */

status_t
store_media( channel_t             channel,
             arc_record_media_t    record,
             size_t                size );

/* -------------------------------------------------------------------------- */

#endif
