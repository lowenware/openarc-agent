#ifndef _AGENT_CONFIGURATION_H_
#define _AGENT_CONFIGURATION_H_

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
  const char * dbName;
  const char * dbUser;
  const char * dbPass;
  const char * dbHost;
  int          dbPort;
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
