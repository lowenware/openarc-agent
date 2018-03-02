#ifndef _AGENT_CHANNEL_H_
#define _AGENT_CHANNEL_H_

#include <stdint.h>
#include "module.h"

/* -------------------------------------------------------------------------- */

#define CHANNEL_FLAG_READY  (1<<0)
#define CHANNEL_FLAG_ERROR  (1<<31)

/* -------------------------------------------------------------------------- */

struct channel
{
  module_t    module;
  void      * m_hd; /* module handler */
  char      * name;
  uint32_t    flags;
  uint32_t    id;
};

typedef struct channel * channel_t;

/* -------------------------------------------------------------------------- */

channel_t
channel_new( uint32_t     id, 
             const char * name,
             const char * uri,
             module_t     module );

/* -------------------------------------------------------------------------- */

status_t
channel_open( channel_t self );

/* -------------------------------------------------------------------------- */

status_t
channel_close( channel_t self );

/* -------------------------------------------------------------------------- */

status_t
channel_recv( channel_t       self,
              arc_record_t ** p_list,
              unsigned int *  size);

/* -------------------------------------------------------------------------- */

status_t
channel_send( channel_t       self,
              arc_command_t   command,
              void          * data,
              unsigned int    size);

/* -------------------------------------------------------------------------- */

void
channel_free( channel_t self );

/* -------------------------------------------------------------------------- */

#endif
