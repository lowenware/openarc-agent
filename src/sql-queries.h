#ifndef _AGENT_SQL_QUERIES_H_
#define _AGENT_SQL_QUERIES_H_

/* -------------------------------------------------------------------------- */

#define SQL_SELECT_CHANNELS                                                   \
    "SELECT "                                                                 \
      "c.id,"                                                                 \
      "c.change_index,"                                                       \
      "c.enabled,"                                                            \
      "c.name,"                                                               \
      "c.uri,"                                                                \
      "m.name "                                                               \
    "FROM "                                                                   \
      "channels c JOIN "                                                      \
      "agents a ON c.agent_id = a.id JOIN "                                   \
      "modules m ON c.module_id = m.id "                                      \
    "WHERE "                                                                  \
      "a.instance=$1 AND "                                                    \
      "c.change_index > $2 AND "                                              \
      "a.enabled=TRUE "                                                       \
    "ORDER BY c.change_index;"

extern const char sqlSelectChannels[];

/* -------------------------------------------------------------------------- */

#endif
