#ifndef _AGENT_COMMON_UTILS_H_
#define _AGENT_COMMON_UTILS_H_

#include <openarc/module-sdk.h>
#include "status.h"

/* -------------------------------------------------------------------------- */

const char *
u_status_to_text( status_t status );

/* -------------------------------------------------------------------------- */

const char *
u_arc_record_to_text( arc_record_t record );

/* -------------------------------------------------------------------------- */

const char *
u_arc_command_to_text( arc_command_t command );

/* -------------------------------------------------------------------------- */

int
u_sock_select(int sd, uint32_t wait_usec, int mode);

#define u_sleep( msec ) u_sock_select( -1, msec*1000, 0)
/* -------------------------------------------------------------------------- */

#endif
