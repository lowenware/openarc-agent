#include <stdlib.h>
#include <cStuff/list.h>
#include "commands-pool.h"



/* -------------------------------------------------------------------------- */

struct pool_item
{
  void           * data;
  channel_t        channel;
  size_t           size;
  arc_command_t    command;
  int              flags;
};

typedef struct pool_item * pool_item_t;

/* -------------------------------------------------------------------------- */

#define POOL_ITEM_FLAG_SENT (1<<0)

/* -------------------------------------------------------------------------- */

static pool_item_t
pool_item_new( arc_command_t    command,
               channel_t        channel,
               size_t           size,
               void           * data )
{
  pool_item_t self;

  if ( (self = malloc(sizeof( struct pool_item ))) != NULL)
  {
    self->command = command;
    self->channel = channel;
    self->size    = size;
    self->data    = data;
  }

  return self;
}

#define pool_item_free free

/* -------------------------------------------------------------------------- */

static list_t gPool;

/* -------------------------------------------------------------------------- */

status_t
commands_pool_init()
{
  if ( !(gPool = list_new(4)) )
    return STATUS_MALLOC_ERROR;

  return STATUS_SUCCESS;
}

/* -------------------------------------------------------------------------- */

void
commands_pool_release()
{
  list_free( gPool, pool_item_free );
}

/* -------------------------------------------------------------------------- */

status_t
commands_pool_add( arc_command_t    command,
                   channel_t        channel,
                   size_t           size,
                   void           * data)
{
  pool_item_t item;

  if ( (item = pool_item_new( command, channel, size, data )) != NULL )
  {
    if (list_append(gPool, item) != -1)
      return STATUS_SUCCESS;

    pool_item_free(item);
  }

  return STATUS_MALLOC_ERROR;
}

/* -------------------------------------------------------------------------- */

status_t
commands_pool_get( arc_command_t  * command,
                   channel_t        channel,
                   size_t         * size,
                   void          ** data)
{
  pool_item_t item;
  int i;

  for (i=0; i<gPool->count; i++)
  {
    item = list_index(gPool, i);
    if (item->channel == channel && ! (item->flags & POOL_ITEM_FLAG_SENT) )
    {
      *size = item->size;
      *data = item->data;

      item->flags |= POOL_ITEM_FLAG_SENT;

      return STATUS_SUCCESS;
    }
  }

  return STATUS_NULL_OBJECT;
}

/* -------------------------------------------------------------------------- */

void
commands_pool_reset( arc_command_t command, channel_t channel, void * data )
{
  pool_item_t item;
  int i;

  for (i=0; i<gPool->count; i++)
  {
    item = list_index(gPool, i);
    if ( item->channel == channel && item->data == data )
    {
      if (item->flags & POOL_ITEM_FLAG_SENT)
        item->flags ^= POOL_ITEM_FLAG_SENT;

      break;
    }
  }
}

/* -------------------------------------------------------------------------- */
