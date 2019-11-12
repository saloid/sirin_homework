#include <stdint.h>
#include <stdbool.h>

#define BUFFER_SIZE 40

void connectToSocket();


void passCommandToDaemon(int argc, char **argv);
void doSomeDaemonStuff();
bool startsWith(const char *pre, const char *str);
void printHelp();
void createSocket();
void processSocket();
int guard(int n, char *err);

//callbacks
void newPacketCallback(uint32_t ipAddr, char *ifaceName);
void signalHandlerCallback(int sig);