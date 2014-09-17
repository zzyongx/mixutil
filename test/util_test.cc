#include <vector>
#include <sys/time.h>
#include <gtest/gtest.h>
#include <mixutil/util.h>

TEST(UtilTest, cs) {
  std::string s;
  ASSERT_EQ(MixUtil::cs(s), s.c_str());

  s.append("abced");
  ASSERT_EQ(strcmp(MixUtil::cs(s), "abced"), 0);
}

TEST(UtilTest, utos) {
  ASSERT_EQ(MixUtil::utos(0), "0");
  ASSERT_EQ(MixUtil::utos(100), "100");
  ASSERT_EQ(MixUtil::utos(901), "901");
  ASSERT_EQ(MixUtil::utos(999), "999");
}

TEST(UtilTest, milliSleep) {
  struct timeval begin, end;
  gettimeofday(&begin, 0);

  MixUtil::milliSleep(100);

  gettimeofday(&end, 0);

  int span = (end.tv_sec - begin.tv_sec) * 1000 +
    (end.tv_usec - begin.tv_usec) / 1000;

  ASSERT_GE(span, 100);
  ASSERT_LE(span, 105);
}

struct Wrap {
  std::string operator()(const std::string &x) const {
    return "<" + x + ">";
  }
};

TEST(UtilTest, join) {
    const char *s[] = { "AA", "BB", "CC", "DD", "EE" };
    std::vector<std::string> sv(s, s+5);

    ASSERT_EQ(MixUtil::join(s, s+5, ":"), "AA:BB:CC:DD:EE");
    ASSERT_EQ(MixUtil::join(sv.begin(), sv.end(), ";", Wrap()),
            "<AA>;<BB>;<CC>;<DD>;<EE>");
}

TEST(UtilTest, split) {
  const char *s = "Aa => ";

  std::string name, value;

  ASSERT_TRUE(MixUtil::split(s, "=>", &name, &value));
  ASSERT_EQ(name, "Aa ");
  ASSERT_EQ(value, " ");

  ASSERT_TRUE(MixUtil::split(s, " =>", &name, &value));
  ASSERT_EQ(name, "Aa");
  ASSERT_EQ(value, " ");

  ASSERT_TRUE(MixUtil::split(s, " => ", &name, &value) == false);
  ASSERT_TRUE(MixUtil::split(s, "=>", &name, &value, " ") == false);

  s = "ABC";

  ASSERT_TRUE(MixUtil::split(s, "", &name, &value));
  ASSERT_EQ(name, "A");
  ASSERT_EQ(value, "BC");

  s = "A B C ";
  ASSERT_TRUE(MixUtil::split(s, "", &name, &value));
  ASSERT_EQ(name, "A");
  ASSERT_EQ(value, " B C ");

  ASSERT_TRUE(MixUtil::split(s, "", &name, &value, " \t"));
  ASSERT_EQ(name, "A");
  ASSERT_EQ(value, "BC");

  s = " ABX    = CDX ";
  ASSERT_TRUE(MixUtil::split(s, "=", &name, &value, " \t"));
  ASSERT_EQ(name, "ABX");
  ASSERT_EQ(value, "CDX");

  s = "ABX=>CDX";
  ASSERT_TRUE(MixUtil::split(s, "=>", &name, &value));
  ASSERT_EQ(name, "ABX");
  ASSERT_EQ(value, "CDX");

  s = "ABX =>";
  ASSERT_TRUE(MixUtil::split(s, "=>", &name, &value) == false);

  s = " =>ABX";
  ASSERT_TRUE(MixUtil::split(s, "=>", &name, &value));
  ASSERT_EQ(name, " ");
  ASSERT_EQ(value, "ABX");

  ASSERT_TRUE(MixUtil::split(s, "=>", &name, &value, " ") == false);

  s = "=>ABX";
  ASSERT_TRUE(MixUtil::split(s, "=>", &name, &value) == false);

  std::vector<std::string> v;

  s = ",,A, ,";

  MixUtil::split(s, ",", std::back_inserter(v));
  ASSERT_EQ(v.size(), 5);
  ASSERT_EQ(v[0], "");
  ASSERT_EQ(v[1], "");
  ASSERT_EQ(v[2], "A");
  ASSERT_EQ(v[3], " ");
  ASSERT_EQ(v[4], "");

  v.clear();

  MixUtil::split(s, ",", std::back_inserter(v), " ");
  ASSERT_EQ(v.size(), 5);
  ASSERT_EQ(v[2], "A");
  ASSERT_EQ(v[3], "");

  v.clear();

  s = "A:B,,C:D, X:Y";
  MixUtil::split(s, ",", std::back_inserter(v), " ");
  ASSERT_EQ(v.size(), 4);
  ASSERT_EQ(v[0], "A:B");
  ASSERT_EQ(v[1], "");
  ASSERT_EQ(v[2], "C:D");
  ASSERT_EQ(v[3], "X:Y");
}

TEST(UtilTest, random) {
  std::string str;

  str = MixUtil::random();
  ASSERT_EQ(str.size(), 8);
  
  for (size_t i = 0; i < 10; i++) {
    str = MixUtil::random(16);
    ASSERT_EQ(str.size(), 16);
    for (size_t j = 0; j < str.size(); j++) {
      ASSERT_TRUE((str[i] >= '0' && str[i] <= '9') ||
                  (str[i] >= 'A' && str[i] <= 'Z') ||
                  (str[i] >= 'a' && str[i] <= 'z'));
    }
  }
}

TEST(UtilTest, crc32) {
  const char *s = "abcdefg";
  ASSERT_EQ(MixUtil::crc32(s, 7), 824863398);
}
