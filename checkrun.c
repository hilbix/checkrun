/* $Header$
 *
 * Simple checker program
 *
 * Copyright (C)2009 Valentin Hilbig <webmaster@scylla-charybdis.com>
 *
 * This is release early code.  Use at own risk.
 *
 * This Works is placed under the terms of the Copyright Less License,
 * see file COPYRIGHT.CLL.  USE AT OWN RISK, ABSOLUTELY NO WARRANTY.
 *
 * Note that the CLL does only cover the code as it is in this file,
 * but THE CLL DOES NOT COVER THE CODE IN THE LIBRARY, the library is
 * mainly GPLv2.
 *
 * $Log$
 * Revision 1.2  2009-01-10 06:35:05  tino
 * Writing to activity file
 *
 */

#define TINO_NEED_OLD_ERR_FN
#include "tino/alarm.h"
#include "tino/proc.h"
#include "tino/ex.h"
#include "tino/getopt.h"

#include "checkrun_version.h"

struct cfg
  {
    const char		*activity, *file, *indicator;
    int			discard, null, progress, verbose;
    unsigned long	max, timeout;

    pid_t		pid;
    int			fd;

    unsigned long	seconds;
    int			mark;
    unsigned long	reads, count;
  };

#define	CFG	struct cfg *cfg

static void
verbose(CFG, int level, const char *s, ...)
{
  tino_va_list	list;

  if (cfg->verbose<level)
    return;
  tino_va_start(list, s);
  tino_vfprintf(stderr, &list);
  tino_va_end(list);
  fprintf(stderr, "\n");
  fflush(stderr);
}


/* Wait a second for termination
 */
static void
term_check(CFG, int timeout)
{
  int	flag;

  flag	= 0;
  for (;;)
    {
      int	status, ret;
      pid_t	child;
      char	*cause;

      child=0;
      if (tino_wait_child_p(&child, (long)(flag ? timeout : 0), &status))
	{
	  if (flag)
	    break;
	  flag	= 1;
	  continue;
	}
      cause	= tino_wait_child_status_string(status, &ret);
      verbose(cfg, 0, "child %ld terminted with status %d: %s", (long)child, status, cause);
      if (child==cfg->pid)
	exit(status);
      tino_freeO(cause);
    }
}

static void
term_kill(CFG, int signal, const char *signame)
{
  term_check(cfg, 1);
  verbose(cfg, -1, "child %ld still running, sending %s", (long)cfg->pid, signame);
  kill(cfg->pid, signal);
  term_check(cfg, 1);
}

static void
fork_child(CFG, char **argv)
{
  int	fds[2];

  if (pipe(fds))
    tino_exit("pipe()");

  cfg->pid	= tino_fork_exec((cfg->null ? tino_file_nullE() : 0), fds[1], 2, argv, NULL, 0, NULL);
  verbose(cfg, 0, "forked child %ld: %s", (long)cfg->pid, *argv);
  tino_file_closeE(fds[1]);

  cfg->fd	= fds[0];
}

static void
alarm_reset(CFG)
{
  TINO_ALARM_RUN();
  cfg->seconds	= 0;
  cfg->mark	= 0;
}

static void
copy_loop(CFG)
{
  alarm_reset(cfg);
  for (;;)
    {
      char	buf[BUFSIZ];
      int	got;

      if ((got=tino_file_readI(cfg->fd, buf, sizeof buf))==0)
	{
	  verbose(cfg, 1, "EOF on input");
	  return;	/* EOF on stdin	*/
	}

      if (got<0)
	{
	  /* Some error	*/
	  if (errno!=EINTR)
	    {
	      tino_err("read input");
	      return;
	    }

	  TINO_ALARM_RUN();
	  if (cfg->mark)
	    {
	      verbose(cfg, 1, "timeout");
	      return;
	    }
	  continue;
	}

      /* Activity	*/
      cfg->reads++;
      cfg->count	+= got;
      verbose(cfg, 2, "activity %d bytes", got);

      if (cfg->file)
	{
	  const void	*tmp;
	  int		out, len;

	  /* Write activity file	*/
	  if ((out=tino_file_open_createE(cfg->file, O_WRONLY|O_APPEND, 0666))<0)
	    {
	      tino_err("cannot create %s", cfg->file);
	      return;
	    }

	  tmp	= buf;
	  len	= got;
	  if (cfg->activity)
	    {
	      tmp	= cfg->activity;
	      len	= strlen(cfg->activity);
	    }
	  if (tino_file_write_allE(out, tmp, len)!=len)
	    {
	      tino_err("write error to activity file %s", cfg->file);
	      return;
	    }
	  if (tino_file_closeE(out))
	    {
	      tino_err("cannot close activity file %s", cfg->file);
	      return;
	    }
	}

      if (cfg->indicator)
	{
	  fprintf(stderr, "%s", cfg->indicator);
	  fflush(stderr);
	}

      if (!cfg->discard && tino_file_write_allE(1, buf, got)!=got)
	{
	  if (errno)
	    tino_err("write output");
	  else
	    verbose(cfg, 1, "EOF on output");
	  return;
	}

      alarm_reset(cfg);
    }
}

static int
alarm_callback(void *user, long delta, time_t now, long run)
{
  struct cfg *cfg = user;

  if (cfg->seconds>=cfg->timeout)
    cfg->mark++;
  else
    cfg->seconds++;

  if (cfg->progress)
    {
      000;
    }
  return 0;
}

int
main(int argc, char **argv)
{
  static struct cfg	cfg;
  int			argn;

  argn	= tino_getopt(argc, argv, 1, 0,
		      TINO_GETOPT_VERSION(CHECKRUN_VERSION)
		      " program [args..]\n"
		      "	Runs program and monitors it's stdout for activity.\n"
		      "	If inactivity timeout is detected the program is terminated.\n"
		      "	Returns 36 on argument errors, 37 on kill problems\n"
		      "	else termination status of forked program (probably killed)"
		      ,

		      TINO_GETOPT_USAGE
		      "h	this help"
		      ,

		      TINO_GETOPT_STRING
		      "a str	Activity string appended to -f (defaults to seen activity)"
		      , &cfg.activity,

		      TINO_GETOPT_FLAG
		      "d	Discard output (=stdout) of program"
		      , &cfg.discard,

		      TINO_GETOPT_STRING
		      "f name	Create file on activity"
		      , &cfg.file,

		      TINO_GETOPT_STRING
		      "i str	Activity indicator (no LF is added) to stderr"
		      , &cfg.indicator,

		      TINO_GETOPT_FLAG
		      "n	Redirect stdin to program from /dev/null"
		      , &cfg.null,
#if 0
		      TINO_GETOPT_ULONGINT
		      TINO_GETOPT_SUFFIX
		      "m max	Show activity progress according to 'max'"
		      , &cfg.max,

		      TINO_GETOPT_FLAG
		      "p	Show progress (activity) counter in regular intervals"
		      , &cfg.progress,
#endif
		      TINO_GETOPT_FLAG
		      TINO_GETOPT_MIN
		      TINO_GETOPT_MAX
		      "q	Quiet mode (less comments)"
		      , &cfg.verbose,
		      2,
		      -2,

		      TINO_GETOPT_ULONGINT
		      TINO_GETOPT_DEFAULT
		      TINO_GETOPT_TIMESPEC
		      "t secs	Inactivity Timeout value (in seconds)"
		      , &cfg.timeout,
		      2ul,

		      TINO_GETOPT_FLAG
		      TINO_GETOPT_MIN
		      TINO_GETOPT_MAX
		      "v	Verbose mode (give more comments)"
		      , &cfg.verbose,
		      -2,
		      2,

		      NULL);

  if (argn<=0)
    return 36;

  fork_child(&cfg, argv+argn);
  tino_alarm_set(1, alarm_callback, &cfg);
  copy_loop(&cfg);
  tino_file_closeE(cfg.fd);
  term_kill(&cfg, SIGHUP,  "HUP");
  term_kill(&cfg, SIGTERM, "TERM");
  term_kill(&cfg, SIGKILL, "KILL");

  verbose(&cfg, -2, "child %ld still running? Unsuccessfully ended (37)", (long)cfg.pid);
  return 37;
}
