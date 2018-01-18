#ifndef _AGENT_CONFIGURATION_H_
#define _AGENT_CONFIGURATION_H_

#include "status.h"

/* -------------------------------------------------------------------------- */

struct configuration
{
  const char * instance;
  const char * modulesRoot;
  const char * systemUser;
  const char * pidFile;
  const char * listenHost;
  int          listenPort;
  const char * logLevel;
  const char * logFile;
  const char * dbDatabase;
  const char * dbUsername;
  const char * dbPassword;
  const char * dbHostname;
  int          dbPort;
  int          dbConnections;
};

typedef struct configuration * configuration_t;

/* -------------------------------------------------------------------------- */

extern configuration_t cfg;

/* -------------------------------------------------------------------------- */

status_t
configuration_init();

/* -------------------------------------------------------------------------- */

void
configuration_log();

/* -------------------------------------------------------------------------- */

status_t
configuration_load_from_file( const char * file );

/* -------------------------------------------------------------------------- */

void
configuration_release();

/* -------------------------------------------------------------------------- */

#endif
