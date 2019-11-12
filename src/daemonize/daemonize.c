#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>

#define LOCKFILE "/var/run/" DAEMON_NAME ".pid"

void skeleton_daemon()
{
	pid_t pid;

	/* Fork off the parent process */
	pid = fork();

	/* An error occurred */
	if (pid < 0)
		exit(EXIT_FAILURE);

	/* Success: Let the parent terminate */
	if (pid > 0)
		exit(EXIT_SUCCESS);

	/* On success: The child process becomes session leader */
	if (setsid() < 0)
		exit(EXIT_FAILURE);

	/* Fork off for the second time*/
	pid = fork();

	/* An error occurred */
	if (pid < 0)
		exit(EXIT_FAILURE);

	/* Success: Let the parent terminate */
	if (pid > 0)
		exit(EXIT_SUCCESS);

	/* Set new file permissions */
	umask(0);

	/* Change the working directory to the root directory */
	/* or another appropriated directory */
	chdir("/");

	/* Close all open file descriptors */
	int x;
	for (x = sysconf(_SC_OPEN_MAX); x >= 0; x--)
	{
		close(x);
	}

	int lockFile = open(LOCKFILE, O_RDWR | O_CREAT, 0640);
	if (lockFile < 0)
	{
		exit(EXIT_FAILURE); /* can not open */
	}
	if (lockf(lockFile, F_TLOCK, 0) < 0)
	{
		exit(EXIT_SUCCESS);
	}
	char str[10];
	sprintf(str, "%d\n", getpid());
	write(lockFile, str, strlen(str)); /* record pid to lockfile */

	
	signal(SIGCHLD, SIG_IGN); /* ignore child */
	signal(SIGTSTP, SIG_IGN); /* ignore tty signals */
	signal(SIGTTOU, SIG_IGN);
	signal(SIGTTIN, SIG_IGN);

	/* Open the log file */
	openlog(DAEMON_NAME, LOG_PID, LOG_DAEMON);
}