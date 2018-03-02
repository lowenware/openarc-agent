#ifndef _AGENT_MODULE_H_
#define _AGENT_MODULE_H_

#include <stdint.h>
#include <openarc/module-sdk.h>
#include "status.h"

/* -------------------------------------------------------------------------- */

struct module
{
  arc_module_alloc_handle_t      alloc_handle;
  arc_module_free_handle_t       free_handle;
  arc_module_set_handle_t        set_handle;
  arc_module_get_error_t         get_error;
  arc_module_confirm_t           confirm;
  arc_module_open_t              open;
  arc_module_close_t             close;
  arc_module_read_t              read;
  arc_module_write_t             write;

  struct arc_version   version;
  struct arc_version   sdkVersion;

  void               * desc;
  char               * name;
  uint32_t             refs;
};

typedef struct module * module_t;

/* -------------------------------------------------------------------------- */

status_t
module_new( module_t * self, const char * name );

/* -------------------------------------------------------------------------- */

void
module_free( module_t self );

/* -------------------------------------------------------------------------- */

#endif
