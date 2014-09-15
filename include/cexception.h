#ifndef _MIXUTIL_CEXCEPTION_H_
#define _MIXUTIL_CEXCEPTION_H_

#include <string>
#include <cstring>
#include <util.h>

namespace MixUtil {

class CException {
 public:
  CException(const std::string &error) {
    error_ = error;
  }

  CException(int eno) {
    error_ = MixUtil::utos(eno) + ":" + strerror(eno);
  }

  CException(const std::string &tips, int eno) {
    error_ = tips + ", " + MixUtil::utos(eno) + ":" + strerror(eno); 
  }

  CException(const std::string &tips, int eno, const std::string &error) {
    error_ = tips + ", " + MixUtil::utos(eno) + ":" + error;
  }

  const char *message() const {
    return MixUtil::cs(error_);
  }

 private:
  std::string error_;
};

} // namespace MixUtil

#endif

