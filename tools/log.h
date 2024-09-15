#ifndef LOG_H
#define LOG_H

#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>
#include <time.h>

#define LOG_VER "0.0.0"

enum {
  TRACE,
  DEBUG,
  INFO,
  WARN,
  ERROR,
  FATAL
};

typedef struct {
  va_list ap;
  const char *fmt;
  const char *file;
  struct tm *time;
  void *udata;
  int line;
  int level;
} logEvent;

typedef void (*logLogFn)(logEvent *ev);
typedef void (*logLockFn)(bool lock, void *udata);

#define logTrace(...) logLog(TRACE, __FILE__, __LINE__, __VA_ARGS__)
#define logDebug(...) logLog(DEBUG, __FILE__, __LINE__, __VA_ARGS__)
#define logInfo(...) logLog(INFO, __FILE__, __LINE__, __VA_ARGS__)
#define logWarn(...) logLog(WARN, __FILE__, __LINE__, __VA_ARGS__)
#define logError(...) logLog(ERROR, __FILE__, __LINE__, __VA_ARGS__)
#define logFatal(...) logLog(FATAL, __FILE__, __LINE__, __VA_ARGS__)

#ifdef __cplusplus
extern "C" {
#endif

const char* logLevelStr(int level);
void logSetLogLevel(int level);
void logSetLock(logLockFn fn, void *udata);
void logSetQuiet(bool enable);
int logAddCallback(logLogFn fn, void *udata, int level);
int logAddFp(FILE *fp, int level);



void logLog(int level, const char *file, int line, const char *fmt, ...);

#ifdef __cplusplus
}
#endif

#endif

