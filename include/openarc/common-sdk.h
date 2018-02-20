#ifndef _ARC_COMMON_SDK_H_
#define _ARC_COMMON_SDK_H_

#include <stdint.h>
#include <sys/types.h>

/* -------------------------------------------------------------------------- */

typedef enum
{
  ARC_STATUS_ERROR               = -1,
  ARC_STATUS_SUCCESS             =  0,
  ARC_STATUS_SUCCESS_WITH_REMARK =  1,
  ARC_STATUS_IDLE                =  2

} arc_status_t;

#define ARC_STATUS_SUCCEEDED( s_code ) (((s_code)&(~1))==0)

/* -------------------------------------------------------------------------- */

struct arc_version
{
  uint16_t major;
  uint16_t minor;
  uint16_t tweak;
  uint16_t build;
};

typedef struct arc_version * arc_version_t;

/* -------------------------------------------------------------------------- */

#define ARC_STRINGIFY(s) #s
#define ARC_TO_STRING(s) ARC_STRINGIFY(s)

/* -------------------------------------------------------------------------- */
#endif
