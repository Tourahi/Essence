#include "log.h"

#define MAX_CALLBACKS 28

static const char *levelStrings[] = {
  "TRACE",
  "DEBUG",
  "INFO",
  "WARN",
  "ERROR",
  "FATAL"
};

#ifdef LOG_USE_COLOR
static const char *levelColors[] = {
  "\x1b[94m", "\x1b[36m", "\x1b[32m", "\x1b[33m", "\x1b[31m", "\x1b[35m"
};
#endif

typedef struct {
  logLogFn fn;
  void *udata;
  int level;
} Callback;

static struct {
  void *udata;
  logLockFn lock;
  int level;
  bool quiet;
  Callback callbacks[MAX_CALLBACKS];
} LOG;

const char* logLevelStr(int level) {
  return levelStrings[level];
}


void logSetLogLevel(int level) {
  LOG.level = level;
}


void logSetLock(logLockFn fn, void *udata) {
  LOG.lock = fn;
  LOG.udata = udata;
}

void logSetQuiet(bool enable) {
  LOG.quiet = enable;
}

int logAddCallback(logLogFn fn, void *udata, int level) {
  for (int i = 0; i < MAX_CALLBACKS; i++) {
    if (!LOG.callbacks[i].fn) {
      LOG.callbacks[i].fn = fn;
      LOG.callbacks[i].udata = udata;
      LOG.callbacks[i].level = level;
      return 0;
    }
  }
  return -1;
}

static void initEvent(logEvent *e, void *udata) {
  if (!e->time) {
    time_t t = time(NULL);
    e->time = localtime(&t);
  }
  e->udata = udata;
}


static void stdoutCallback(logEvent *e) {
  char buf[16];
  buf[strftime(buf, sizeof(buf), "%H:%M:%S", e->time)] = '\0';
#ifdef LOG_USE_COLOR
  fprintf(
    e->data, "%s %s%-5s\x1b[0m \x1b[90m%s:%d:\x1b[0m ",
    buf, levelColors[e->level], levelStrings[e->level],
    e->file, e->line);
#else
  fprintf(
    (FILE*) e->udata, "%s %-5s %s:%d: ",
    buf, levelStrings[e->level], e->file, e->line);
#endif
  vfprintf((FILE*) e->udata, e->fmt, e->ap);
  fprintf((FILE*) e->udata, "\n");
  fflush((FILE*) e->udata);
}

static void fileCallback(logEvent *e) {
  char buf[64];
  buf[strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", e->time)] = '\0';
  fprintf(
    (FILE*) e->udata, "%s %-5s %s:%d: ",
    buf, levelStrings[e->level], e->file, e->line);
  vfprintf((FILE*) e->udata, e->fmt, e->ap);
  fprintf((FILE*) e->udata, "\n");
  fflush((FILE*) e->udata);
}

int logAddFp(FILE *fp, int level) {
  return logAddCallback(fileCallback, fp, level);
}


static void lock(void) {
  if (LOG.lock) { LOG.lock(true, LOG.udata); }
}

static void unlock(void) {
  if (LOG.lock) { LOG.lock(false, LOG.udata); }
}

void logLog(int level, const char *file, int line, const char *fmt, ...) {
  logEvent e = {
    .fmt   = fmt,
    .file  = file,
    .line  = line,
    .level = level,
  };

  lock();

  if (!LOG.quiet && level >= LOG.level) {
    initEvent(&e, stderr);
    va_start(e.ap, fmt);
    stdoutCallback(&e);
    va_end(e.ap);
  }

  for (int i = 0; i < MAX_CALLBACKS && LOG.callbacks[i].fn; i++) {
    Callback *cb = &LOG.callbacks[i];
    if (level >= cb->level) {
      initEvent(&e, cb->udata);
      va_start(e.ap, fmt);
      cb->fn(&e);
      va_end(e.ap);
    }
  }

  unlock();
}