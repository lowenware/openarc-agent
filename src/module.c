#include <stdlib.h>
#include <dlfcn.h>
#include <unistd.h>
#include <cStuff/str-utils.h>
#include <cStuff/log.h>

#include "configuration.h"
#include "module.h"

static
const char errNoSymInMod[]     = "no symbol '%s' in '%s' module",
           cSymAllocHandle[]   = ARC_TO_STRING( ARC_MODULE_ALLOC_HANDLE ),
           cSymFreeHandle[]    = ARC_TO_STRING( ARC_MODULE_FREE_HANDLE ),
           cSymSetHandle[]     = ARC_TO_STRING( ARC_MODULE_SET_HANDLE ),
           cSymGetError[]      = ARC_TO_STRING( ARC_MODULE_GET_ERROR ),
           cSymConfirmRecord[] = ARC_TO_STRING( ARC_MODULE_CONFIRM_RECORD ),
           cSymOpen[]          = ARC_TO_STRING( ARC_MODULE_OPEN ),
           cSymClose[]         = ARC_TO_STRING( ARC_MODULE_CLOSE ),
           cSymRead[]          = ARC_TO_STRING( ARC_MODULE_READ ),
           cSymWrite[]         = ARC_TO_STRING( ARC_MODULE_WRITE ),

           cSymGetVersion[]    = ARC_TO_STRING( ARC_MODULE_GET_VERSION ),
           cSymGetSdkVersion[] = ARC_TO_STRING( ARC_MODULE_GET_SDK_VERSION );


/* -------------------------------------------------------------------------- */

/* 
 * #define GET_FUNC( type, name )  ((type) dlsym((*self)->desc, (sym = name)))
 * */

/* *(void **) (&funcp) = dlsym(libHandle, argv[2]); */

#define GET_FUNC(res, name) (*(void**)(&(res))=dlsym((*self)->desc, (sym=name)))
/* -------------------------------------------------------------------------- */

status_t
module_new( module_t * self, const char * name )
{
  status_t     result = STATUS_SUCCESS;
  char       * mod_file;
  const char * sym;

  arc_module_get_version_t        get_version;

  if ( !(mod_file = str_printf("%s/%s.so", cfg->modulesRoot, name)) )
    return STATUS_MALLOC_ERROR;

  if ( access( mod_file, F_OK) != 0 )
  {
    result = STATUS_NOT_FOUND;
    goto finally;
  }



  if ( !(*self = calloc(1, sizeof(struct module))) )
  {
    result = STATUS_MALLOC_ERROR;
    goto finally;
  }

  if ( !((*self)->name = str_copy(name)) )
  {
    result = STATUS_MALLOC_ERROR;
    goto release_self;
  }

  if ( !((*self)->desc = dlopen( mod_file, RTLD_LAZY )) )
  {
    log_debug("dlopen() = %s", dlerror());
    result = STATUS_SYSCALL_ERROR;
    goto release_self;
  }

  /* see GET_FUNC macro: shorter call + pointer's magic to avoid compiler
   *                     warnings
   * */
  if ( ! GET_FUNC((*self)->alloc_handle, cSymAllocHandle) )
    goto e_dlsym;

  if ( ! GET_FUNC((*self)->free_handle, cSymFreeHandle) )
    goto e_dlsym;

  if ( ! GET_FUNC((*self)->set_handle, cSymSetHandle) )
    goto e_dlsym;

  if ( ! GET_FUNC((*self)->get_error, cSymGetError) )
    goto e_dlsym;

  if ( ! GET_FUNC((*self)->confirm_record, cSymConfirmRecord) )
    goto e_dlsym;

  if ( ! GET_FUNC((*self)->open, cSymOpen) )
    goto e_dlsym;

  if ( ! GET_FUNC((*self)->close, cSymClose) )
    goto e_dlsym;

  if ( ! GET_FUNC((*self)->read, cSymRead) )
    goto e_dlsym;

  if ( ! GET_FUNC((*self)->write, cSymWrite) )
    goto e_dlsym;

  if ( ! GET_FUNC(get_version, cSymGetVersion) )
    goto e_dlsym;

  get_version( &(*self)->version );

  if ( ! GET_FUNC(get_version, cSymGetSdkVersion) )
    goto e_dlsym;

  get_version( &(*self)->sdkVersion );

  goto finally;

e_dlsym:
  log_debug(errNoSymInMod, sym, name);
  result = STATUS_EXTCALL_ERROR;

release_self:
  module_free(*self);
  *self = NULL;

finally:
  free(mod_file);
  return result;
}

/* -------------------------------------------------------------------------- */

void
module_free( module_t self )
{
  if (self->desc)
    dlclose(self->desc);

  if(self->name)
    free(self->name);

  free(self);
}

/* -------------------------------------------------------------------------- */
