#ifndef _AGENT_STORE_CONTACT_ID_H_
#define _AGENT_STORE_CONTACT_ID_H_

#include <openarc/module-sdk.h>

#include "status.h"
#include "channel.h"

/* -------------------------------------------------------------------------- */

status_t
store_contact_id( channel_t                  channel,
                  arc_record_contact_id_t    record,
                  size_t                     size );

/* -------------------------------------------------------------------------- */

#endif
