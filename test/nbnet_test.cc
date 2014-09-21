#include <time.h>
#include <unordered_map>
#include <gtest/gtest.h>
#include <mysql/mysql.h>
#include <mixutil/nbnet.h>
#include <sys/epoll.h>

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
  //std::cout << "DELETE: " << query << "\n";
  ASSERT_EQ(mysql_query(db, query), 0);


  sprintf(query, "insert test(name) values('%s')", name);
  //std::cout << "INSERT: " << query << "\n";  
  ASSERT_EQ(mysql_query(db, query), 0) << mysql_error(db);

  sprintf(query, "select id, name from test where name = '%s' and %s",
          name, "IF(sleep(1), TRUE, TRUE)");
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

void ctl_handler_()
{
  int fd = epoll_create(128);
  std::unordered_map<int, bool> hash;

  for ( ;; ) {
    size_t size = nbnet_events_size();
    std::cout << "size:" << size << "\n";
    
    if (size == 0) {
      pth_cancel_point();
      pth_yield(NULL);
      continue;
    }

    nbnet_io_event *events = new nbnet_io_event[size];
    nbnet_events(events);

    for (size_t i = 0; i < size; ++i) {
      std::string evs;
      int ev = 0;
      if (events[i].event & NBNET_EVENT_READ) {
        ev |= EPOLLIN;
        evs += "read";
      }
      if (events[i].event & NBNET_EVENT_WRITE) {
        ev |= EPOLLOUT;
        evs += ",write";
      }
      if (ev == 0) continue;


      struct epoll_event event;
      event.events = ev;
      event.data.ptr = &events[i];

      if (hash.count(events[i].fd)) {
        epoll_ctl(fd, EPOLL_CTL_MOD, events[i].fd, &event);
      } else {
        pth_suspend(events[i].tid);
        hash[events[i].fd] = true;

        pth_attr_t attr = pth_attr_of(events[i].tid);
        const char *name;
        pth_attr_get(attr, PTH_ATTR_NAME, &name);
        std::cout << "suspend: " << name
                  << " fd: " << events[i].fd
                  << " wait: " << evs << "\n";
        
        epoll_ctl(fd, EPOLL_CTL_ADD, events[i].fd, &event);
      }
    }

    struct epoll_event *epoll_events = new epoll_event[size];
    
    int nn = epoll_wait(fd, epoll_events, size, 1000);
    
    if (nn < 0) abort();
    
    for (int i = 0; i < nn; i++) {
      nbnet_io_event *event = (nbnet_io_event *) epoll_events[i].data.ptr;
      epoll_ctl(fd, EPOLL_CTL_DEL, event->fd, NULL);
      hash.erase(event->fd);
      
      pth_attr_t attr = pth_attr_of(events[i].tid);
      const char *name;
      pth_attr_get(attr, PTH_ATTR_NAME, &name);
      std::cout << "resume: " << name << "\n";

      pth_resume(event->tid);
    }
    pth_yield(NULL);
  }
}

void* ctl_handler(void *)
{
  ctl_handler_();
  return NULL;
}

void mysql_(bool autorun)
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
    
    ASSERT_TRUE((tids[i] = pth_spawn(attr, NbNetTest::mysql_handler, name[i])));
  }

  pth_t ctl = NULL;
  if (!autorun) {
    ctl = pth_spawn(NULL, ctl_handler, NULL);
  }
    
  for (int i = 0; i < N; i++) {
    pth_join(tids[i], NULL);
  }
  
  if (ctl) {
    pth_cancel(ctl);
    pth_join(ctl, NULL);
  }
}

TEST_F(NbNetTest, mysql) {  
  time_t start, block, auto_nb, manual_nb;

  nbnet_mode_ctl(NBNET_BLOCK);
  
  start = time(0);
  mysql_(true);
  block = time(0) - start;

  nbnet_mode_ctl(NBNET_AUTO_NONBLOCK);
  
  start = time(0);
  mysql_(true);
  auto_nb = time(0) - start;

  nbnet_mode_ctl(NBNET_MANUAL_NONBLOCK);
  start = time(0);
  mysql_(false);
  manual_nb = time(0) - start;

  std::cout << "block:"           << block << "\n"
            << "auto nonblock:"   << auto_nb << "\n"
            << "manual nonblock:" << manual_nb << "\n";
}
