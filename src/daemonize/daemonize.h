#ifndef _DAEMONIZE_H_
#define _DAEMONIZE_H_

#include "config.h"
#include <stdbool.h>

#define LOCKFILE "/var/run/" DAEMON_NAME ".pid"

/*
 * Forks process to create daemon (if it's possible and no daemon running now).
 * Retrurns true if current process daemon, false - if not. Exits program if some errors occured 
 */
bool createDaemon();

#endif //_DAEMONIZE_H_