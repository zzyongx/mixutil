#include <util.h>

namespace MixUtil {

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


