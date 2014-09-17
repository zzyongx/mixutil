#include <cstdlib>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

#include <mixutil/util.h>

namespace MixUtil {

std::string random(size_t len)
{
  bool useDev = false;
  std::string s(len, '\0');
  
  int fd = open("/dev/urandom", O_RDONLY);
  if (fd > 0) {
    size_t n = 0;
    ssize_t nn = 0;
    while ((nn = read(fd, const_cast<char*>(s.data()+n), len-n)) > 0) {
      n += nn;
      if (len == n) break;
    }
    if (nn > 0) {
      useDev = true;
      close(fd);
    }
  }

  if (!useDev) {
    for (size_t i = 0; i < len; ++i) {
      s[i] = rand();
    }
  }

  for (size_t i = 0; i < len; ++i) {
    int c = s[i] % 62;
    if (c < 0) c = -c;
    
    if (c < 10) {
      s[i] = c + '0';
    } else if (c < 36) {
      s[i] = c - 10 + 'A';
    } else {
      s[i] = c - 36 + 'a';
    }
  }
  return s;
}    

bool split(const std::string &s, const std::string &sp, std::string *name,
           std::string *value, const std::string &trimChars)
{
  name->clear();
  value->clear();

  std::string *p = name;

  for (size_t i = 0; i < s.size(); ++i) {
    if (p == name) {
      if (sp.empty()) {
        if (i != 0) {
          p = value;
        }
      } else if (s.find(sp, i) == i){
        i += sp.size()-1;
        p = value;
        continue;
      }
    }

    if (trimChars.find(s[i]) == std::string::npos) {
      p->append(1, s[i]);
    }
  }

  /* if p point to name, not value exist, split failed */
  return (name->empty() || value->empty()) ? false : true;
}

} // namespace MixUtil


