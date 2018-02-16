#include <stdlib.h>
#include <sys/select.h>
#include "common-utils.h"

/* -------------------------------------------------------------------------- */

const char *
u_status_to_text( status_t status )
{
  switch(status)
  {
    case STATUS_ACCESS_DENIED:       return "access denied";

    /* application level */   
    case STATUS_DATABASE_ERROR:      return "database error"; 
    case STATUS_CONFIGURATION_ERROR: return "configuration error";
    case STATUS_INIT_ERROR:          return "init error";

    /* daemon level */        
    case STATUS_PID_EXISTS:          return "PID exists";
    case STATUS_USAGE_ERROR:         return "usage error";

    /* environment level */   
    case STATUS_PARSE_ERROR:         return "parse error";
    case STATUS_NOT_FOUND:           return "not found";
    case STATUS_EXTCALL_ERROR:       return "external call error";
    case STATUS_SYSCALL_ERROR:       return "system call error";
    case STATUS_MALLOC_ERROR:        return "memory allocation error";
    case STATUS_NULL_OBJECT:         return "null object";

    /* positives */           
    case STATUS_SUCCESS:             return "success";
    case STATUS_SUCCESS_WITH_REMARK: return "success with remark";
    case STATUS_PENDING:             return "pending";
  }

  return "unknown status";
}

/* -------------------------------------------------------------------------- */

const char *
u_arc_record_to_text( arc_record_t record )
{

  switch( record )
  {
    case ARC_RECORD_NULL:                 return "Null-Record";
    case ARC_RECORD_CLIENT_CONNECTED:     return "Client Connected";
    case ARC_RECORD_CLIENT_DISCONNECTED:  return "Client Disconnected";
    case ARC_RECORD_HEARTBEAT:            return "Heartbeat";
    case ARC_RECORD_CONTACT_ID:           return "Contact ID";
    case ARC_RECORD_LOCATION:             return "Location";
    case ARC_RECORD_MEDIA:                return "Media";
  }

  return "unknown record";
}

/* -------------------------------------------------------------------------- */

const char *
u_arc_command_to_text( arc_command_t command )
{
  switch( command )
  {
    case ARC_MODULE_SET_TIMEZONE: return "SET_TIMEZONE";
    case ARC_MODULE_SET_TIME:     return "SET_TIME";
    case ARC_MODULE_ARM:          return "ARM";
    case ARC_MODULE_DISARM:       return "DISARM";
    case ARC_MODULE_CALLBACK:     return "CALLBACK";
  }

  return "unknown command";
}

/* -------------------------------------------------------------------------- */
