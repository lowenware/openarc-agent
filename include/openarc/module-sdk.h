#ifndef _OPENARC_MODULE_SDK_H_
#define _OPENARC_MODULE_SDK_H_

#include <openarc/common-sdk.h>


/* -------------------------------------------------------------------------- */

#define ARC_SDK_VERSION_MAJOR 1
#define ARC_SDK_VERSION_MINOR 0
#define ARC_SDK_VERSION_TWEAK 0
#define ARC_SDK_VERSION_BUILD 0

#define ARC_SDK_VERSION { ARC_SDK_VERSION_MAJOR, \
                          ARC_SDK_VERSION_MINOR, \
                          ARC_SDK_VERSION_TWEAK, \
                          ARC_SDK_VERSION_BUILD  }

/* -------------------------------------------------------------------------- */

/* functions names */

#define ARC_MODULE_READ            mod_read
#define ARC_MODULE_WRITE           mod_write
#define ARC_MODULE_GET_VERSION     mod_get_version
#define ARC_MODULE_GET_SDK_VERSION mod_get_sdk_version

/* -------------------------------------------------------------------------- */

#define ARC_MODULE_PROTOCOL        "protocol"
#define ARC_MODULE_USERNAME        "username"
#define ARC_MODULE_PASSWORD        "password"
#define ARC_MODULE_ADDRESS         "address"
#define ARC_MODULE_PORT            "port"
#define ARC_MODULE_PATH            "path"

/* -------------------------------------------------------------------------- */

typedef enum
{
  ARC_RECORD_NULL       = 0,
  ARC_RECORD_HEARTBEAT  = 100,
  ARC_RECORD_CONTACT_ID = 200,
  ARC_RECORD_LOCATION   = 300

} arc_record_t;

/* -------------------------------------------------------------------------- */

typedef enum
{
  ARC_COMMAND_ALLOCATE,        /* allocate channel handler */
  ARC_COMMAND_FREE,            /* free resources used by channel handler */
  ARC_COMMAND_CONFIGURE,       /* configure channel handler */
  ARC_COMMAND_OPEN,            /* open channel */
  ARC_COMMAND_CLOSE,           /* close channel */
  ARC_COMMAND_GET_ERROR,       /* get last error */
  ARC_COMMAND_GET_ID,          /* get last command id */
  ARC_COMMAND_CONFIRM,         /* confirm record */
  ARC_COMMAND_RESET            /* reset record */

} arc_command_t;



/* -------------------------------------------------------------------------- */

typedef void
(*arc_module_get_version_t) (arc_version_t version);

/* -------------------------------------------------------------------------- */

typedef arc_status_t
(*arc_module_read_t) (  void          * mod_handler,
                        arc_record_t  * record,
                        size_t        * count,
                        void          * data );

/* -------------------------------------------------------------------------- */

typedef arc_status_t
(*arc_module_write_t) ( void          * mod_handler,
                        arc_command_t   command,
                        size_t          count,
                        void          * data );

/* -------------------------------------------------------------------------- */

struct arc_record_heartbeat
{
  char        * received; /* always utc */
  uint32_t      callsign;
  char          gsm_level;
  char          bat_level;
};

typedef struct arc_record_heartbeat * arc_record_heartbeat_t;

/* -------------------------------------------------------------------------- */

struct arc_record_location
{
  char        * occured;  /* device timezone */
  char        * received; /* always utc */
  char        * code;
  uint32_t      callsign;
  long double   latitude;
  long double   longitude;
  double        altitude;
  float         speed;
  float         direction;
  char          gsm_level;
  char          bat_level;
};

typedef struct arc_record_location * arc_record_location_t;

/* -------------------------------------------------------------------------- */

struct arc_record_contact_id
{
  char        * occured;        /* device timezone */
  char        * received;       /* always utc */
  uint32_t      account;        /* account id */
  uint16_t      code;           /* event code */
  uint16_t      zone;           /* zone code */
  uint8_t       partition;      /* partition code */
  uint8_t       level;          /* signal level */
};

typedef struct arc_record_contact_id * arc_record_contact_id_t;

/* -------------------------------------------------------------------------- */

#define ARC_MODULE_DEFINE( VERSION_MAJOR,                                   \
                           VERSION_MINOR,                                   \
                           VERSION_TWEAK,                                   \
                           VERSION_BUILD )                                  \
                                                                            \
  struct arc_version mod_sdk_version = ARC_SDK_VERSION;                     \
  struct arc_version mod_version     = { VERSION_MAJOR,                     \
                                         VERSION_MINOR,                     \
                                         VERSION_TWEAK,                     \
                                         VERSION_BUILD };                   \
                                                                            \
  void                                                                      \
  ARC_MODULE_GET_SDK_VERSION( arc_version_t version )                       \
  {                                                                         \
    memcpy(version, &mod_sdk_version, sizeof(struct arc_version));          \
  }                                                                         \
                                                                            \
  void                                                                      \
  ARC_MODULE_GET_VERSION( arc_version_t version )                           \
  {                                                                         \
    memcpy(version, &mod_version, sizeof(struct arc_version));              \
  }                                                                         \

/* -------------------------------------------------------------------------- */

#endif
