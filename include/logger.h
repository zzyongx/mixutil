#ifndef _MIXUTIL_LOGGER_H_
#define _MIXUTIL_LOGGER_H_

#include <cstdio>
#include <cerrno>
#include <cstring>
#include <string>
#include <memory>
#include <stdint.h>
#include <stdarg.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>

#include <util.h>
#include <cexception.h>

namespace MixUtil {

class Logger {
 public:
  Logger(const std::string &file, time_t *now = 0) : file_(file), now_(now) {
    handler_ = open(Util::cs(file_), O_WRONLY | O_APPEND | O_CREAT,
                    S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH); 
    if (handler_ == -1) throw ExceptionC("Logger::open", errno);
    reopen_  = false;
    verbose_ = false;
  }

  ~Logger() {
    close(handler_);
  }

  static const uint8_t LOG_FATAL = 5;
  static const uint8_t LOG_ERROR = 4;
  static const uint8_t LOG_DEBUG = 3;

  bool log(uint8_t level, int eno, const char *fmt, ... ) {
    if (reopen_) {
      int fd = open(Util::cs(file_), O_WRONLY | O_APPEND | O_CREAT,
                    S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
      if (fd != -1) {
        close(handler_);
        handler_ = fd;
      }
      reopen_ = false;
    }

    /* one more for '\n' */
    char errstr[ERR_STR + 1];
    size_t n = 0;

    if (level == LOG_FATAL || level == LOG_ERROR) {
      struct tm ltm;
      time_t now = now_ ? *now_ : time(0);
      n = snprintf(errstr, ERR_STR, "[%s] ", level == LOG_FATAL ? "fatal" : "error");
      n += strftime(errstr + n, ERR_STR - n,
                    "%Y-%m-%d %H:%M:%S ", localtime_r(&now, &ltm));
      n += snprintf(errstr + n, ERR_STR - n,
                    "%d:%s ", eno, (eno == 0) ? "" : strerror(eno));
    } else if (verbose_) {
      struct timeval now;
      gettimeofday(&now, 0);
      n = snprintf(errstr, ERR_STR, "[debug] %lu.%lu %d:%s ",
                   static_cast<unsigned long>(now.tv_sec),
                   static_cast<unsigned long>(now.tv_usec),
                   eno, (eno == 0) ? "" : strerror(eno));
    } else {
      return true;
    }

    va_list ap;

    va_start(ap, fmt);
    n += vsnprintf(errstr + n, ERR_STR - n, fmt, ap);
    va_end(ap);

    if (n >= ERR_STR) n = ERR_STR;
    errstr[n++] = '\n';

    return (write(handler_, errstr, n) != -1);
  }

  void reopen(bool aReopen) { reopen_ = aReopen; }

  void verbose(bool aVerbose) { verbose_ = aVerbose; }
  bool verbose() const { return verbose_; }

 private:
  static const size_t ERR_STR = 2047;

  std::string file_;
  int handler_;

  time_t *now_;

  volatile bool reopen_;
  volatile bool verbose_;
};

} // namespace MixUtil

#ifdef NO_LOGGER

# define log_fatal(eno, fmt, args...) printf(fmt"\n", ##args)
# define log_error(eno, fmt, args...) printf(fmt"\n", ##args)
# define log_debug(eno, fmt, args...) printf(fmt"\n", ##args)

#else

extern std::auto_ptr<Util::Logger> logger;

# define log_fatal(eno, fmt, args...) logger->log(Util::Logger::LOG_FATAL, eno, fmt, ##args)
# define log_error(eno, fmt, args...) logger->log(Util::Logger::LOG_ERROR, eno, fmt, ##args)
# define log_debug(eno, fmt, args...) logger->log(Util::Logger::LOG_DEBUG, eno, fmt, ##args)

#endif

#endif


