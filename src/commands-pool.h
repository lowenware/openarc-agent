#ifndef _AGENT_COMMANDS_POOL_H_
#define _AGENT_COMMANDS_POOL_H_

#include <openarc/module-sdk.h>
#include "status.h"
#include "channel.h"

/* -------------------------------------------------------------------------- */

status_t
commands_pool_init();

/* -------------------------------------------------------------------------- */

void
commands_pool_release();

/* -------------------------------------------------------------------------- */

status_t
commands_pool_add( arc_command_t    command,
                   channel_t        channel,
                   size_t           size,
                   void           * data);

/* -------------------------------------------------------------------------- */

status_t
commands_pool_get( arc_command_t  * command,
                   channel_t        channel,
                   size_t         * size,
                   void          ** data);

/* -------------------------------------------------------------------------- */

void
commands_pool_reset( arc_command_t command, channel_t channel, void * data );

/* -------------------------------------------------------------------------- */


#endif
