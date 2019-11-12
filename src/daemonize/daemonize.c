#include "daemonize.h"
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
#include <stdbool.h>

bool createDaemon()
{
	pid_t pid;

	pid = fork(); // Fork off the parent process

	if (pid < 0) //	fork not succefull
	{
		exit(EXIT_FAILURE);
	}

	if (pid > 0) // Success, let main process continue work
	{
		return (false);
	}

	if (setsid() < 0) // On success: The child process becomes session leader
	{
		exit(EXIT_FAILURE);
	}

	pid = fork(); // More forks for the fork God 

	if (pid < 0) //	fork not succefull
	{
		exit(EXIT_FAILURE);
	}

	if (pid > 0) // Success: Let the parent terminate
	{
		exit(EXIT_SUCCESS);
	}

	umask(0);   // Set new file permissions
	chdir("/"); // Change the working directory to the root directory

	for (int x = sysconf(_SC_OPEN_MAX); x >= 0; x--) // Close all open file descriptors (stdin, out, err)
	{ 
		close(x);
	}

	int lockFile = open(LOCKFILE, O_RDWR | O_CREAT, 0640);	//creating lock file 
	if (lockFile < 0)
	{
		exit(EXIT_FAILURE); // file open/creation error
	}
	if (lockf(lockFile, F_TLOCK, 0) < 0)	// Locking file
	{
		exit(EXIT_SUCCESS);	// File was already locked - another process exists
	}

	char str[10];
	sprintf(str, "%d\n", getpid());
	write(lockFile, str, strlen(str)); // record pid to lockfile

	signal(SIGCHLD, SIG_IGN); // ignore some signals
	signal(SIGTSTP, SIG_IGN);
	signal(SIGTTOU, SIG_IGN);
	signal(SIGTTIN, SIG_IGN);

	openlog(DAEMON_NAME, LOG_PID, LOG_DAEMON);	//start logging

	return (true);
}