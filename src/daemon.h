#ifndef _AGENT_DAEMON_H_
#define _AGENT_DAEMON_H_

#include <cStuff/version.h>
#include "status.h"

/* return values ------------------------------------------------------------ */


#define DAEMON_FLAG_TERMINATE      (1 << 0)
#define DAEMON_FLAG_RELOAD         (1 << 1)
#define DAEMON_FLAG_DATABASE_READY (1 << 2)
#define DAEMON_FLAG_DATABASE_ERROR (1 << 3)
#define DAEMON_FLAG_LOAD_CHANNELS  (1 << 4)
#define DAEMON_FLAG_WAIT_CHANNELS  (1 << 5)


/* -------------------------------------------------------------------------- */

extern version_t  gVersion;
extern const char gVersionLabel[];

/* -------------------------------------------------------------------------- */

typedef status_t
(* daemon_loop_t)(int * flags);

/* -------------------------------------------------------------------------- */

status_t
daemon_run(int argc, char ** argv, daemon_loop_t loop_run);

/* -------------------------------------------------------------------------- */

#endif
