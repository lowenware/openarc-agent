/* main.c
 * */
#include <libintl.h>
#include <locale.h>

#include "daemon.h"
#include "loop.h"


int
main(int argc, char ** argv)
{
  /* localization */
  bindtextdomain (APPLICATION, LOCALE_PATH);
  bind_textdomain_codeset (APPLICATION, "UTF-8");
  textdomain (APPLICATION);

  /* execution */
  return daemon_run( argc, argv, loop_run );
}

