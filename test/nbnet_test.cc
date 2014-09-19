#include <time.h>
#include <gtest/gtest.h>
#include <mysql/mysql.h>
#include <pth.h>

/* db: test
 * table: CREATE TABLE test (id INT PRIMARY KEY AUTO_INCREMENT, name VARCHAR(32));
 * user: zzyong, passwd: 123456, host: 127.0.0.1, port:3306
 */

const char *user   = "zzyong";
const char *pass   = "123456";
const char *dbname = "test";
const char *host   = "127.0.0.1";
int port           = 3306;

class NbNetTest: public ::testing::Test {
public:
  NbNetTest() {
    // initialization code here
  }

  void SetUp() {
    // code here will execute just before the test ensues
    pth_init();
  }

  void TearDown( ) {
    // code here will be called just after the test completes
    pth_kill();
  }

  ~NbNetTest()  {
    // cleanup any pending stuff, but no exceptions allowed
  }

  static void *mysql_handler(void *data) {
    mysql_handler_(data);
    return NULL;
  }
  static void mysql_handler_(void *data);

  // put in any custom data members that you need
};
void NbNetTest::mysql_handler_(void *data)
{
  char *name = static_cast<char *>(data);
  MYSQL *db = mysql_init(0);
  ASSERT_TRUE(mysql_real_connect(db, host, user, pass, dbname, port, 0, 0))
    << mysql_error(db);

  char query[256];

  sprintf(query, "delete from test where name = '%s'", name);
  std::cout << "DELETE: " << query << "\n";
  ASSERT_EQ(mysql_query(db, query), 0);


  sprintf(query, "insert test(name) values('%s')", name);
  std::cout << "INSERT: " << query << "\n";  
  ASSERT_EQ(mysql_query(db, query), 0) << mysql_error(db);

  sprintf(query, "select id, name from test where name = '%s'", name);
  std::cout << "SELECT: " << query << "\n";
  ASSERT_EQ(mysql_query(db, query), 0);

  MYSQL_RES *result;
  ASSERT_TRUE((result = mysql_store_result(db)));

  ASSERT_EQ(mysql_num_rows(result), 1);

  MYSQL_ROW row = mysql_fetch_row(result);
  ASSERT_EQ(std::string(row[1]), name);

  std::cout <<"ID " << row[0] << ": " << row[1] << "\n";
  mysql_free_result(result);

  mysql_close(db);
}

int nbnet_block_ctl(int);

void mysql_()
{
  const int N = 100;
  char name[N][8];
  pth_t tids[N];
  
  for (int i = 0; i < N; i++) {
    sprintf(name[i], "name#%d", i);
    
    pth_attr_t attr = pth_attr_new();
    pth_attr_set(attr, PTH_ATTR_JOINABLE, TRUE);
    pth_attr_set(attr, PTH_ATTR_STACK_SIZE, 64 * 1024);
    pth_attr_set(attr, PTH_ATTR_NAME, name[i]);
    
    ASSERT_TRUE((tids[i] = pth_spawn(NULL, NbNetTest::mysql_handler, name[i])));
  }

  for (int i = 0; i < N; i++) {
    pth_join(tids[i], NULL);
  }
 
}
TEST_F(NbNetTest, mysql) {  
  time_t fast, slow, start;
  
  start = time(0);
  mysql_();
  fast = time(0) - start;

  nbnet_block_ctl(1);
  
  start = time(0);
  mysql_();
  slow = time(0) - start;

  std::cout << "nonblock:" << fast << "\n"
            << "block:"    << slow << "\n";
}
