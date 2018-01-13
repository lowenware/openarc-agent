#ifndef _AGENT_DAEMON_H_
#define _AGENT_DAEMON_H_

#include <cStuff/version.h>
#include "status.h"

/* return values ------------------------------------------------------------ */


#define DAEMON_FLAG_TERMINATE 1
#define DAEMON_FLAG_RELOAD    2

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
