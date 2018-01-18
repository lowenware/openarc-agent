#ifndef _AGENT_STATUS_H_
#define _AGENT_STATUS_H_

#include <cStuff/retcodes.h>

/* type --------------------------------------------------------------------- */

typedef enum
{
  /* site level */
  STATUS_ACCESS_DENIED                 = -500,

  /* application level */
  STATUS_DATABASE_ERROR                = -302,
  STATUS_CONFIGURATION_ERROR           = -301,
  STATUS_INIT_ERROR                    = -300,

  /* daemon level */
  STATUS_PID_EXISTS                    = -201,
  STATUS_USAGE_ERROR                   = -200,

  /* environment level */
  STATUS_PARSE_ERROR                   = CSTUFF_PARSE_ERROR,
  STATUS_NOT_FOUND                     = CSTUFF_NOT_FOUND,
  STATUS_EXTCALL_ERROR                 = CSTUFF_EXTCALL_ERROR,
  STATUS_SYSCALL_ERROR                 = CSTUFF_SYSCALL_ERROR,
  STATUS_MALLOC_ERROR                  = CSTUFF_MALLOC_ERROR,
  STATUS_NULL_OBJECT                   = CSTUFF_NULL_OBJECT,

  /* positives */
  STATUS_SUCCESS                       = CSTUFF_SUCCESS,
  STATUS_SUCCESS_WITH_REMARK           = CSTUFF_SUCCESS_WITH_REMARK,
  STATUS_PENDING                       = CSTUFF_PENDING

} status_t;

/* -------------------------------------------------------------------------- */

#define STATUS_SUCCEEDED( s_code ) (((s_code)&(~1))==0)

/* -------------------------------------------------------------------------- */

#endif
