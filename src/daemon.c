#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <pwd.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <syslog.h>

#include <cStuff/log.h>
#include <cStuff/fs-utils.h>

#include "configuration.h"
#include "daemon.h"

/* -------------------------------------------------------------------------- */

version_t  gVersion        = { VERSION_MAJOR,
                               VERSION_MINOR,
                               VERSION_TWEAK,
                               VERSION_BUILD };

const char gVersionLabel[] =   VERSION_LABEL;

/* -------------------------------------------------------------------------- */

static int    gFlags   = 0;
char        * gCfgFile = NULL;

/* ersion-------------------------------------------------------------------------- */

const char msgRunHelp[]   = "run '"PROJECT_NAME" help' for more information\n",
           msgOnKill[]    = "signal %d has been sent to daemon with PID %d\n",
           msgSignal[]    = "%s received\n",
           msgStarted[]   = "started",
           msgStopped[]   = "stopped",
           errOnStart[]   = "failed to start daemon\n",
           errOnStop[]    = "failed to stop daemon\n",
           errOnKill[]    = "SIGQUIT for PID %d failed (%s)\n",
           errNoUser[]    = "user not found",
           errMalloc[]    = "out of memory",
           errLogFile[]   = "failed to output log into file %s (%s)\n",
           errFuncStr[]   = "%s(%s) failed (%s)\n",
           errFuncInt[]   = "%s(%d) failed (%s)\n",
           errLoadPid[]   = "failed to load PID from file %s (%s)\n",
           errSavePid[]   = "failed to save PID to file %s (%s)\n",
           errPidExists[] = "daemon already running with PID %d\n",
           errSigSet[]    = "failed to handle %s signal\n",
           errNoCmd[]     = "command not specified\n",
           errBadOpt[]    = "unknown option '%s'\n",
           errBadCmd[]    = "unknown command '%s'\n";

/* -------------------------------------------------------------------------- */

const char cmdStart[]  = "start",
           cmdStop[]   = "stop",
           cmdHelp[]   = "help",
           cmdReload[] = "reload";

/* -------------------------------------------------------------------------- */

const char argCfgFile[]    = "--config-file=",
           argDaemonMode[] = "--daemon-mode";

/* -------------------------------------------------------------------------- */

const char defCfgFile[] = CONFIG_PATH;

/* -------------------------------------------------------------------------- */

static int
daemon_release( int ret )
{
  remove( cfg->pidFile );
  configuration_release();

  gFlags |= DAEMON_FLAG_TERMINATE;

  return ret;
}

/* -------------------------------------------------------------------------- */

static int
daemon_init_failure( int ret, const char * format, ... )
{
  va_list vl;

  va_start( vl, format );
  vfprintf( stderr, format, vl );
  va_end( vl );

  fputs( errOnStart, stderr );

  return daemon_release( ret );
}

/* -------------------------------------------------------------------------- */

static int
daemon_start_failure( int ret, const char * format, ... )
{
  va_list vl;

  va_start( vl, format );
  log_vprintf( &stdlog, LOG_LEVEL_ERROR, format, vl );
  va_end( vl );

  log_error( errOnStart );

  return daemon_release( ret );
}

/* -------------------------------------------------------------------------- */

static void
daemon_on_get_signal( int signum )
{
  switch( signum )
  {
    case SIGHUP:
      gFlags |= DAEMON_FLAG_RELOAD;
      gFlags |= DAEMON_FLAG_TERMINATE;
      break;
    case SIGPIPE:
      log_state(msgSignal, "SIGPIPE");
      break;
    case SIGQUIT:
      log_alert(msgSignal, "SIGQUIT");
      gFlags |= DAEMON_FLAG_TERMINATE;
      break;
    case SIGINT:
      log_alert(msgSignal, "SIGINT");
      break;
    case SIGTERM:
      log_alert(msgSignal, "SIGTERM");
      exit(STATUS_SUCCESS_WITH_REMARK); /* TERMINATED */
  }
}

/* -------------------------------------------------------------------------- */

static pid_t
daemon_get_pid_from_file( const char * pidFile )
{
  int     l;
  pid_t   pid;
  FILE  * f = fopen(pidFile, "r");
  char    pidbuf[16];

  if (!f) return -1;
  
  memset(pidbuf, 0, sizeof(pidbuf));

  l = fread(pidbuf, 1, sizeof(pidbuf), f);
  pid = (l>0) ? strtol(pidbuf, NULL, 10) : 0;

  fclose(f);
  return pid;
}

/* -------------------------------------------------------------------------- */

static int
daemon_start(int go_bg, daemon_loop_t loop_run )
{
  status_t            result;
  int                 i;
  FILE              * f;
  struct passwd     * pw        = NULL;
  struct sigaction    sa;
  pid_t               pid;
  uid_t               uid       = getuid();
  const char          io_null[] = "/dev/null";

  if ( (result = configuration_init()) != STATUS_SUCCESS )
  {
    fputs( errOnStart, stderr );
    return result;
  }

  if (configuration_load_from_file( gCfgFile ) != 0)
  {
    fputs( errOnStart, stderr );
    return daemon_release( STATUS_CONFIGURATION_ERROR );
  }

  /* check pid */
  if ( access( cfg->pidFile, F_OK) != -1)
  {
    pid = daemon_get_pid_from_file( cfg->pidFile );

    if (kill(pid, 0) != -1 || errno == EPERM)
    {
      return daemon_init_failure(STATUS_PID_EXISTS, errPidExists, pid);
    }

    remove(cfg->pidFile);
  }

  /* check user */
  if ( uid == 0 && *cfg->systemUser )
  {
    errno = 0;
    if ( (pw = getpwnam( cfg->systemUser )) == NULL )
      return daemon_init_failure(
               STATUS_SYSCALL_ERROR, errFuncStr, "getpwnam", cfg->systemUser,
               errno ? strerror(errno) : errNoUser
             );
  }

  log_set_level( &stdlog, cfg->logLevel );
  stdlog.file = cfg->logFile;

  /* create log */
  if ( stdlog.level && *stdlog.file )
  {
    if (fs_make_file_path( cfg->logFile ) == -1)
    {
      return daemon_init_failure(
               STATUS_INIT_ERROR, errLogFile, cfg->logFile,
               errno ? strerror(errno) : errMalloc
             );
    }

    if (pw && uid != pw->pw_uid)
    {
      if (chown(cfg->logFile, pw->pw_uid, -1) == -1)
      {
        return daemon_init_failure(
          STATUS_INIT_ERROR, errLogFile, cfg->logFile, strerror(errno)
        );
      }
    }
  }

  configuration_log();

  if (go_bg)
  {
    if ( (pid = fork()) < 0 )
      return daemon_start_failure(
               STATUS_SYSCALL_ERROR, errFuncStr, "fork", "", strerror(errno)
             );

    if (pid > 0)
      exit(daemon_release( STATUS_SUCCESS )); /* quit parent process */

    if ( (pid = setsid()) < 0 )
      return daemon_start_failure(
               STATUS_SYSCALL_ERROR, errFuncStr, "setsid", "", strerror(errno)
             );

    if ( (pid = fork()) < 0 )
      return daemon_start_failure(
               STATUS_SYSCALL_ERROR, errFuncStr, "fork", "", strerror(errno)
             );

    if (pid > 0)
      exit(daemon_release( STATUS_SUCCESS )); /* quit first child process */

    if ( chdir("/") != 0 )
      return daemon_start_failure(
               STATUS_SYSCALL_ERROR, errFuncStr, "chdir", "/", strerror(errno)
             );

    umask(0);

    pid = getpid();

    if ( (f = fopen( cfg->pidFile, "w" )) == NULL)
      return daemon_start_failure(
               STATUS_SYSCALL_ERROR, errSavePid, cfg->pidFile, strerror(errno)
             );

    fprintf( f, "%d", pid );
    fclose(f);

    memset (&sa, '\0', sizeof(sa));
    sa.sa_handler = daemon_on_get_signal;
    sa.sa_flags   = SA_RESTART;
    sigemptyset(&sa.sa_mask);
    sigaddset(&sa.sa_mask, SIGHUP);
    sigaddset(&sa.sa_mask, SIGTERM);
    sigaddset(&sa.sa_mask, SIGINT);
    sigaddset(&sa.sa_mask, SIGPIPE);
    sigaddset(&sa.sa_mask, SIGQUIT);

    if (sigaction(SIGQUIT, &sa, NULL) == -1)
      return daemon_start_failure(
               STATUS_SYSCALL_ERROR, errSigSet, "SIGQUIT", strerror(errno)
             );

    if (sigaction(SIGHUP, &sa, NULL) == -1)
      return daemon_start_failure(
               STATUS_SYSCALL_ERROR, errSigSet, "SIGHUP", strerror(errno)
             );

    if (sigaction(SIGTERM, &sa, NULL) == -1)
      return daemon_start_failure(
               STATUS_SYSCALL_ERROR, errSigSet, "SIGTERM", strerror(errno)
             );

    if (sigaction(SIGINT, &sa, NULL) == -1) 
      return daemon_start_failure(
               STATUS_SYSCALL_ERROR, errSigSet, "SIGINT", strerror(errno)
             );

    if (sigaction(SIGPIPE, &sa, NULL) == -1) 
      return daemon_start_failure(
               STATUS_SYSCALL_ERROR, errSigSet, "SIGPIPE", strerror(errno)
             );

    /* close all open file descriptors */
    for( i=sysconf(_SC_OPEN_MAX); i>=0; --i )
    {
      close(i);
    }

    /* reset stdin, stdout, stderr */
    stdin  = fopen(io_null, "r") ;  /* fd=0 */
    stdout = fopen(io_null, "w+");  /* fd=1 */
    stderr = fopen(io_null, "w+");  /* fd=2 */
  }

  /* switch user */
  if ( pw && (uid != pw->pw_uid) && (setuid( pw->pw_uid ) == -1) )
  {
      return daemon_start_failure(
               STATUS_SYSCALL_ERROR, errFuncInt, "setuid", pw->pw_uid,
               strerror(errno)
             );
  }

  openlog( PROJECT_NAME, LOG_PID, LOG_DAEMON );
  syslog (LOG_NOTICE, msgStarted);
  i = daemon_release( loop_run( &gFlags ) );
  if ( gFlags & DAEMON_FLAG_RELOAD )
  {
    gFlags ^= DAEMON_FLAG_RELOAD;
    gFlags ^= DAEMON_FLAG_TERMINATE;
  }
  syslog (LOG_NOTICE, msgStopped);
  closelog();

  return i;
}

/* -------------------------------------------------------------------------- */

static int
daemon_kill( int signum )
{
  status_t result;

  if ( (result=configuration_init()) != STATUS_SUCCESS )
    return result;

  if (configuration_load_from_file( gCfgFile ) != 0)
  {
    fputs( errOnStop, stderr );
    return STATUS_CONFIGURATION_ERROR;
  }

  pid_t pid = daemon_get_pid_from_file( cfg->pidFile );

  if (pid != -1)
  {
    if (kill(pid, signum) == 0)
    {
      printf( msgOnKill, signum, pid );
      return STATUS_SUCCESS;
    }

    fprintf( stderr, errOnKill, pid, strerror(errno) );
  }
  else
    fprintf( stderr, errLoadPid, cfg->pidFile, strerror(errno) );

  fputs( errOnStop, stderr );

  return STATUS_SYSCALL_ERROR;
}
/* -------------------------------------------------------------------------- */

static int
daemon_reload()
{
  return daemon_kill( SIGHUP );
}

/* -------------------------------------------------------------------------- */

static int
daemon_stop()
{
  return daemon_kill( SIGQUIT );
}

/* -------------------------------------------------------------------------- */

static int
daemon_help()
{
  printf( PROJECT_NAME " version %d.%d.%d.%d (%s)\n"
          "USAGE:\n"
          "  "PROJECT_NAME" [--config-file=FILE] [--daemon-mode] "
             "(start|stop|reload|help)\n"
          "OPTIONS:\n"
          "  --config-file=FILE : path to configuration file (%s)\n"
          "  --daemon-mode      : run in daemon mode\n"
          "COMMANDS:\n"
          "  start  : start daemon\n"
          "  stop   : stop daemon\n"
          "  reload : gracefuly reload daemon\n"
          "  help   : show this information\n",
          gVersion.major,
          gVersion.minor,
          gVersion.tweak,
          gVersion.build,
          gVersionLabel,
          gCfgFile
  );

  return STATUS_SUCCESS;
}

/* -------------------------------------------------------------------------- */

static status_t
daemon_usage_failure( const char * format, ... )
{
  va_list vl;

  va_start( vl, format );
  vfprintf(stderr, format, vl);
  va_end( vl );

  fputs(msgRunHelp, stderr);

  return STATUS_USAGE_ERROR;
}

/* -------------------------------------------------------------------------- */

status_t
daemon_run( int argc, char ** argv, daemon_loop_t loop_run )
{
  int i,
      go_bg = 0;

  if ( !(--argc > 0) )
    return daemon_usage_failure( errNoCmd );

  gCfgFile = (char *) defCfgFile;

  for(i=1; i<argc; i++)
  {
    if ( strncmp(argv[i], argCfgFile, sizeof(argCfgFile)) == 0 )
      gCfgFile=&argv[i][ sizeof(argCfgFile) ];
    else if ( strcmp( argv[i], argDaemonMode ) == 0 )
      go_bg = 1;
    else
      return daemon_usage_failure( errBadOpt, argv[i] );
  }

  if ( strcmp( argv[ argc ], cmdStart  )==0 )
  {
    i = STATUS_SUCCESS;
    while ( ! (gFlags & DAEMON_FLAG_TERMINATE)) /* reloading loop */
    {
      i = daemon_start( go_bg, loop_run );
    }
    return i;
  }
  else
  if ( strcmp( argv[ argc ], cmdStop   )==0 ) return daemon_stop();       else
  if ( strcmp( argv[ argc ], cmdReload )==0 ) return daemon_reload();     else
  if ( strcmp( argv[ argc ], cmdHelp   )==0 ) return daemon_help();

  return daemon_usage_failure( errBadCmd, argv[ argc ] );
}

/* -------------------------------------------------------------------------- */
