#ifndef _OPENARC_MODULE_SDK_H_
#define _OPENARC_MODULE_SDK_H_

#include <arpa/inet.h>
#include <openarc/common-sdk.h>


/* -------------------------------------------------------------------------- */

#define ARC_SDK_VERSION_MAJOR 1
#define ARC_SDK_VERSION_MINOR 1
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

#define ARC_MODULE_ALLOC_HANDLE    mod_alloc_handle
#define ARC_MODULE_FREE_HANDLE     mod_free_handle
#define ARC_MODULE_SET_HANDLE      mod_set_handle
#define ARC_MODULE_GET_ERROR       mod_get_error
#define ARC_MODULE_CONFIRM_RECORD  mod_confirm_record
#define ARC_MODULE_OPEN            mod_open
#define ARC_MODULE_CLOSE           mod_close


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
  ARC_RECORD_NULL                = 0,
  ARC_RECORD_HEARTBEAT           = 100,
  ARC_RECORD_CLIENT_CONNECTED    = 200,
  ARC_RECORD_CLIENT_DISCONNECTED = 210,
  ARC_RECORD_CONTACT_ID          = 300,
  ARC_RECORD_LOCATION            = 400,
  ARC_RECORD_MEDIA               = 500

} arc_record_t;

/* -------------------------------------------------------------------------- */

typedef enum
{
  ARC_MODULE_SET_TIMEZONE,
  ARC_MODULE_SET_TIME,
  ARC_MODULE_ARM,
  ARC_MODULE_DISARM,
  ARC_MODULE_CALLBACK

} arc_command_t;

/* -------------------------------------------------------------------------- */

typedef void
(*arc_module_get_version_t) (   arc_version_t   version);

/* -------------------------------------------------------------------------- */

typedef arc_status_t
(*arc_module_alloc_handle_t)(   void         ** p_handle );

/* -------------------------------------------------------------------------- */

typedef arc_status_t
(*arc_module_free_handle_t)(    void          * handle );

/* -------------------------------------------------------------------------- */

typedef arc_status_t
(*arc_module_set_handle_t)(     void          * handle,
                                size_t          count,
                                char         ** p_list );

/* -------------------------------------------------------------------------- */

typedef arc_status_t
(*arc_module_get_error_t)(      void          * handle, 
                                const char   ** error_message);

/* -------------------------------------------------------------------------- */

typedef arc_status_t
(*arc_module_confirm_record_t)( void          * handle, 
                                void          * p_record, 
                                int             value);

/* -------------------------------------------------------------------------- */

typedef arc_status_t
(*arc_module_open_t)(           void          * handle );

/* -------------------------------------------------------------------------- */

typedef arc_status_t
(*arc_module_close_t)(          void          * handle );

/* -------------------------------------------------------------------------- */

typedef arc_status_t
(*arc_module_read_t) (          void          * handle,
                                arc_record_t  * record_type,
                                size_t        * count,
                                void          * record );

/* -------------------------------------------------------------------------- */

typedef arc_status_t
(*arc_module_write_t) (         void          * handle,
                                arc_command_t   command,
                                size_t          count,
                                void          * data );

/* -------------------------------------------------------------------------- */

struct arc_record_client
{
  struct sockaddr_in    address;
  uint64_t              callsign;
};

typedef struct arc_record_client * arc_record_client_t;

/* -------------------------------------------------------------------------- */

struct arc_record_heartbeat
{
  char        * received; /* always utc */
  char        * code;
  uint32_t      callsign;
  int8_t        gsmLevel;
  int8_t        batLevel;
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
  int8_t        gsmLevel;
  int8_t        batLevel;
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
  int8_t        partition;      /* partition code */
  int8_t        gsmlevel;          /* signal level */
};

typedef struct arc_record_contact_id * arc_record_contact_id_t;

/* -------------------------------------------------------------------------- */

struct arc_record_media
{
  char        * received;       /* always utc */
  char        * data;
  char        * type;
  char        * code;
  size_t        size;
};

typedef struct arc_record_media * arc_record_media_t;

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
  __attribute__ ((visibility ("default") ))                                 \
  void                                                                      \
  ARC_MODULE_GET_SDK_VERSION( arc_version_t version )                       \
  {                                                                         \
    memcpy(version, &mod_sdk_version, sizeof(struct arc_version));          \
  }                                                                         \
                                                                            \
  __attribute__ ((visibility ("default") ))                                 \
  void                                                                      \
  ARC_MODULE_GET_VERSION( arc_version_t version )                           \
  {                                                                         \
    memcpy(version, &mod_version, sizeof(struct arc_version));              \
  }                                                                         \

/* -------------------------------------------------------------------------- */

#endif
