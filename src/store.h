#ifndef _AGENT_STORE_H_
#define _AGENT_STORE_H_

#include <stdbool.h>
#include <openarc/module-sdk.h>
#include <sys/types.h>
#include <cStuff/dbx.h>
#include "channel.h"
#include "status.h"

/* -------------------------------------------------------------------------- */

status_t
store_records( channel_t channel, arc_record_t * list, unsigned int size );

/* -------------------------------------------------------------------------- */

#endif
