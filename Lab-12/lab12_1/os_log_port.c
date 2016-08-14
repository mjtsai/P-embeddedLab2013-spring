/*
 * os_log.c port for uC/OS-II on OpenRISC.
 */

#include "os_log_port.h"

#include <includes.h>
#include <sys/stat.h>
#include <unistd.h>

#if OS_ARG_CHK_EN > 0
#define LOG_ASSERT( b , ...) \
  if ( !(b) ) { \
    logFilePrint(stdout,  "ASSERT @ %s # %d, ", __FILE__, __LINE__); \
    logFilePrint(stdout, __VA_ARGS__); \
    logFilePrint(stdout, "\n"); \
    exit(0);\
  };
#else
#define LOG_ASSERT(...) ((void) 0) 
#endif

/*
 * File handling.
 */

static int fileOpened = 0;

LogFileHandle logFileOpen(const char *path, const char *mode) {
  /* check if another file is already open */
  if (fileOpened == 0) {
    fileOpened = 1;
    printf("<<<<< BEGIN %s >>>>>\n", path);
    return (LogFileHandle)1;
  } else {
    return (LogFileHandle)0;
  }
}

int logFileClose(LogFileHandle fp) {
  if (fileOpened > 0) {
    printf("<<<<< END >>>>>\n");
    fileOpened = 0;
    return 1;
  } else {
    printf("FILE NOT OPENED %d\n", fileOpened);
    return 0;
  }
  return 0;
}
