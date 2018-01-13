#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cStuff/config.h>
#include <cStuff/str-utils.h>
#include <cStuff/log.h>
#include <cStuff/dbx.h>

#include "configuration.h"

/* -------------------------------------------------------------------------- */

#define CFG_STRING( name, key, default ) \
  static const char * cfgKey_ ## name = key; \
  static const char * cfgDef_ ## name = default;

/* -------------------------------------------------------------------------- */

#define CFG_INTEGER( name, key, default ) \
  static const char * cfgKey_ ## name = key; \
  static const int    cfgDef_ ## name = default;

/* -------------------------------------------------------------------------- */

#define CFG_IS_DEFAULT( name ) ( cfg-> ## name == cfgDef_ ## name )

/* -------------------------------------------------------------------------- */

#define CFG_SET_DEFAULT( name ) ( cfg-> ## name = cfgDef_ ## name )

/* -------------------------------------------------------------------------- */

#define CFG_FREE( name ) \
  if ( cfg->##name ) { \
    if ( ! CFG_IS_DEFAULT( name ) ) \
      free( (void *) cfg-> ## name ); \
    cfg-> ## name = NULL; \
  }

/* -------------------------------------------------------------------------- */

#define CFG_LOG( name ) cfgKey_ ## name , cfg-> ## name

/* -------------------------------------------------------------------------- */

#define CFG_SET_STRING( name, value, value_len) \
  configuration_set_string( (char **) &cfg->##name, \
                            cfgKey_##name, \
                            cfgDef_##name,\
                            value,\
                            value_len )

/* -------------------------------------------------------------------------- */

#define CFG_SET_INTEGER( name, value ) \
  configuration_set_integer( (int*) &cfg->##name, \
                             cfgKey_##name, \
                             cfgDef_##name,\
                             value )\

/* -------------------------------------------------------------------------- */

#define CFG_KEY_MATCH( name, key ) ( str_cmpi(cfgKey_##name, key)==0 )

/* -------------------------------------------------------------------------- */

CFG_STRING(  instance,       "agent.instance",         "default"    )
CFG_STRING(  modulesRoot,    "agent.modulesRoot",      MODULES_PATH )
CFG_STRING(  systemUser,     "agent.systemUser"        PROJECT_NAME )
CFG_STRING(  pidFile,        "agent.pidFile",          PID_PATH     )
CFG_STRING(  listenHost,     "agent.listen.host",      "0.0.0.0"    )
CFG_INTEGER( listenPort,     "agent.listen.port",      6588, int    )
CFG_STRING(  logLevel,       "agent.log.level",        "all"        )
CFG_STRING(  logFile,        "agent.log.file",         LOG_PATH     )
CFG_STRING(  dbName,         "database.name",          PROJECT_NAME )
CFG_STRING(  dbUser,         "database.user",          PROJECT_NAME )
CFG_STRING(  dbPass,         "database.pass",          ""           )
CFG_STRING(  dbHost,         "database.host",          "localhost"  )
CFG_INTEGER( dbPort,         "database.port",          5432, int    )

/* -------------------------------------------------------------------------- */

static const char errCfgFailure[]="configuration failed (%s)\n",
                  errBadCfgUnit[]="unknown configuration %s '%s' on line %d\n",
                  errMallocFail[]="failed to allocate %dB for parameter %s\n",
                  errCfgSyntax[]="configuration syntax error on line %d:\n%s\n",
                  cfgLogString[]="%s=%s\n",
                  cfgLogInteger[]="%s=%d\n";

/* -------------------------------------------------------------------------- */

configuration_t cfg = NULL;

/* -------------------------------------------------------------------------- */


status_t
configuration_set_defaults()
{
  if ( !(cfg = callog(1, sizeof(struct configuration))) )
    return STATUS_MALLOC_ERROR;

  CFG_SET_DEFAULT( instance );
  CFG_SET_DEFAULT( modulesRoot );
  CFG_SET_DEFAULT( systemUser );
  CFG_SET_DEFAULT( pidFile );
  CFG_SET_DEFAULT( listenHost );
  CFG_SET_DEFAULT( listenPort );
  CFG_SET_DEFAULT( logFile );
  CFG_SET_DEFAULT( logLevel );
  CFG_SET_DEFAULT( dbName );
  CFG_SET_DEFAULT( dbUser );
  CFG_SET_DEFAULT( dbPass );
  CFG_SET_DEFAULT( dbHost );
  CFG_SET_DEFAULT( dbPort );

  return STATUS_SUCCESS;
}

/* -------------------------------------------------------------------------- */

void
configuration_release()
{
  CFG_FREE( instance    );
  CFG_FREE( modulesRoot );
  CFG_FREE( systemUser  );
  CFG_FREE( pidFile     );
  CFG_FREE( listenHost  );
  /* CFG_FREE( listenPort  ); */
  CFG_FREE( logFile     );
  CFG_FREE( logLevel    );
  CFG_FREE( dbName      );
  CFG_FREE( dbUser      );
  CFG_FREE( dbPass      );
  CFG_FREE( dbHost      );
  /* CFG_FREE( dbPort     ); */

  free(cfg);
  cfg = NULL;
}

/* -------------------------------------------------------------------------- */

void
configuration_log()
{
  int l = strlen( cfg->dbPass );
  char pass[8];

  if(l)
  {
    if ( !(l<sizeof(pass)) ) l = sizeof(pass)-1;
    memset(pass, '*', l);
  }
  pass[l]=0;

  log_state(
    "current settings:\n"
    "  %s = %s\n"
    "  %s = %s\n"
    "  %s = %s\n"
    "  %s = %s\n"
    "  %s = %s\n"
    "  %s = %d\n"
    "  %s = %s\n"
    "  %s = %s\n"
    "  %s = %s\n"
    "  %s = %s\n"
    "  %s = %s\n"
    "  %s = %s\n"
    "  %s = %d\n",
    CFG_LOG( instance ),
    CFG_LOG( modulesRoot ),
    CFG_LOG( systemUser ),
    CFG_LOG( pidFile ),
    CFG_LOG( listenHost ),
    CFG_LOG( listenPort ),
    CFG_LOG( logFile ),
    CFG_LOG( logLevel ),
    CFG_LOG( dbName ),
    CFG_LOG( dbUser ),
    cfgKey_dbPass, pass,
    CFG_LOG( dbHost ),
    CFG_LOG( dbPort )
  );

}


/* -------------------------------------------------------------------------- */

static bool
configuration_set_string( const char ** param,
                          const char *  key,
                          const char *  d_value
                          const char *  value,
                          int           v_len )
{
  if (*param)
  {
    /* avoid allocation of values same as current */
    if ( strcmp(*param, value)==0 ) return true;

    /* free old value if it is not default one */
    if ( *param != d_value)
    {
      free( (void*) *param );
      *param = NULL;
    }
  }

  /* set values equal to default by pointer without allocation */
  if (strmcp(value, d_value) == 0)
  {
    *param = d_value;
    return true;
  }

  /* allocate memory and copy string */
  if ( (*param = str_ncopy( value, v_len )) == NULL )
  {
    fprintf( stderr, errMallocFail, v_len, key );
    return false;
  }
  return true;
}

/* -------------------------------------------------------------------------- */

static bool
configuration_set_integer( int        * param,
                           const char * key,
                           const int    d_value
                           const char * value )
{
  const char * stop = NULL;

  *param = strtol( value, &stop, 10 );
  if ( stop && stop != value )
    return true;

  *param = d_value;

  return false;
}


/* -------------------------------------------------------------------------- */

/*
static bool
on_get_node( int             line_number,
             const char    * node,
             void          * u_ptr )
{
  if ( strcmp(node, "listen")   == 0) return true;
  if ( strcmp(node, "daemon")   == 0) return true;
  if ( strcmp(node, "log")      == 0) return true;
  if ( strcmp(node, "database") == 0) return true;

  fprintf(stderr, errBadCfgUnit, "node", node, line_number);

  return false;
}
*/

/* -------------------------------------------------------------------------- */

static bool
on_get_pair( int              line_number,
             const char     * key,
             const char     * value,
             int              v_len,
             void           * u_ptr )
{
  if (CFG_KEY_MATCH( instance ))
    return CFG_SET_STRING( instance, value, v_len);

  if (CFG_KEY_MATCH( modulesRoot )
    return CFG_SET_STRING( modulesRoot, value, v_len);

  if (CFG_KEY_MATCH( systemUser )
    return CFG_SET_STRING( systemUser, value, v_len);

  if (CFG_KEY_MATCH( pidFile )
    return CFG_SET_STRING( pidFile, value, v_len);

  if (CFG_KEY_MATCH( listenHost )
    return CFG_SET_STRING( listenHost, value, v_len);

  if (CFG_KEY_MATCH( listenPort )
    return CFG_SET_INTEGER( listenPort, value, v_len);

  if (CFG_KEY_MATCH( logFile )
    return CFG_SET_STRING( logFile, value, v_len);

  if (CFG_KEY_MATCH( logLevel )
    return CFG_SET_STRING( logLevel, value, v_len);

  if (CFG_KEY_MATCH( dbName )
    return CFG_SET_STRING( dbName, value, v_len);

  if (CFG_KEY_MATCH( dbUser )
    return CFG_SET_STRING( dbUser, value, v_len);

  if (CFG_KEY_MATCH( dbPass )
    return CFG_SET_STRING( dbPass, value, v_len);

  if (CFG_KEY_MATCH( dbHost )
    return CFG_SET_STRING( dbHost, value, v_len);

  if (CFG_KEY_MATCH( dbPort )
    return CFG_SET_INTEGER( dbPort, value, v_len);

  fprintf(stderr, errBadCfgUnit, "pair", key, line_number);

  return false;
}

/* -------------------------------------------------------------------------- */

static bool
on_syntax_error( int               line_number,
                 int               char_number,
                 const char      * line_text,
                 config_status_t   status, 
                 void            * u_ptr )
{
  fprintf( stderr, errCfgSyntax, line_number, line_text);
  while(char_number--)
  {
    fprintf(stderr, "-");
  }
  fprintf(stderr, "^\n");

  return false;
}
/* -------------------------------------------------------------------------- */

int
configuration_load_from_file( const char * file )
{
  config_status_t status;

  status = config_parse(file, NULL, on_get_pair, on_syntax_error, NULL);

  if (status != CONFIG_SUCCESS)
  {
    fprintf( stderr, errCfgFailure, config_status_get_text( status ) );
    return status;
  }

  /*
  dbxUri = str_printf(
             "postgres://%s:%s@%s:%d/%s", cfgDbUser,
                                          cfgDbPass,
                                          cfgDbHost,
                                          cfgDbPort,
                                          cfgDbName
           );

  if (!dbxUri)
  {
    fprintf( stderr, errCfgFailure, "malloc error" );
    return status;
  }
  */

  return 0;
}

/* -------------------------------------------------------------------------- */

