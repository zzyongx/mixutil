#ifndef _MIXUTIL_H_
#define _MIXUTIL_H_

#include <cstdio>
#include <cstring>
#include <string>
#include <algorithm>
#include <stdint.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <sys/types.h>
#include <pthread.h>

namespace MixUtil {

inline const char *cs(const std::string &str)
{
  return str.c_str();
}

inline uintptr_t uintptr(const void *p)
{
  return reinterpret_cast<uintptr_t>(p);
}

inline std::string utos(uint64_t u)
{
  std::string s;
  s.reserve(sizeof("18446744073709551616"));
  do {
    int t = u % 10; 
    s.append(1, t+48);
    u /= 10; 
  } while (u != 0); 

  std::reverse(s.begin(), s.end());
  return s;
}
 
inline int milliSleep(int ms) 
{
  struct timespec req = { 0, ms * 1000000 };
  return nanosleep(&req, 0);
}

std::string random(size_t len = 8);

template <typename T>
struct NullJoinWrap {
  const std::string &operator()(const T &x) const {
    return x;
  }
};

template <typename Iterator, class JoinWrap>
std::string join(Iterator begin, Iterator end, const std::string &sp, JoinWrap wrap)
{
  std::string s;
  for (Iterator ite = begin; ite != end; ++ite) {
    if (s.empty()) {
      s.assign(wrap(*ite));
    } else {
      s.append(sp).append(wrap(*ite));
    }
  }

  return s;
}
 
template <typename Iterator>
inline std::string join(Iterator begin, Iterator end, const std::string &sp)
{
  return join(begin, end, sp, NullJoinWrap<std::string>());
}

  /* if sp is in trimChars, set needTrim = false
   */
bool split(const std::string &s, const std::string &sp, std::string *name,
           std::string *value, const std::string &trimChars = std::string());

template <typename BackInserter>
void split(const std::string &s, const std::string &sp, BackInserter ite,
           const std::string &trimChars = std::string())
{
  if (s.empty()) return;

  std::string ele;
  for (size_t i = 0; i < s.size(); ++i) {
    if (sp.empty()) {
      if (i != 0) {
        *ite = ele;
        ele.clear();
      }
    } else if (s.find(sp, i) == i) {
      i += sp.size()-1;
      *ite = ele;
      ele.clear();
      continue;
    }
    if (trimChars.find(s[i]) == std::string::npos) {
      ele.append(1, s[i]);
    }
  }

  *ite = ele;
}

uint32_t crc32(const char *key, size_t key_length);

struct SigHandler {
  int signum;
  void (*handler)(int);
};

inline bool setupSigHandler(const SigHandler *sh, size_t n)
{
  struct sigaction sa;

  for (size_t i = 0; i < n; ++i) {
    memset(&sa, 0x00, sizeof(sa));
    sa.sa_handler = sh[i].handler;
    sigemptyset(&sa.sa_mask);
    sigaction(sh[i].signum, &sa, NULL);
  }
  return true;
}

class AutoMutex {
 public:
  AutoMutex(pthread_mutex_t *mutex) : mutex_(mutex) {
    pthread_mutex_lock(mutex_);
  }

  ~AutoMutex() {
    pthread_mutex_unlock(mutex_);
  }

 private:
  pthread_mutex_t *mutex_;
};

class PidFile {
 public:
  PidFile(const std::string &file) : file_(file) {
    FILE *fp = fopen(cs(file_), "w"); 
    if (fp) {
      fprintf(fp, "%d", static_cast<int>(getpid()));
      fclose(fp);
    }
  }

  ~PidFile() {
    unlink(cs(file_));
  }

 private:
  std::string file_;
};

class NoCopyAssign {
 public:
  NoCopyAssign() { }
 private:
  NoCopyAssign(const NoCopyAssign &x);
  NoCopyAssign &operator=(const NoCopyAssign &x);
};

}  // MixUtil

#endif

