/* author: iamzhengzhiyong@gmail.com
 */
#include <pth.h>
#include <mixutil/index_hash.h>

#ifdef __cplusplus
extern "C" {
#endif

#define NBNET_BLOCK           0
#define NBNET_AUTO_NONBLOCK   1
#define NBNET_MANUAL_NONBLOCK 2
int nbnet_mode_ctl(int);  

#define NBNET_EVENT_READ  0x01
#define NBNET_EVENT_WRITE 0x02
struct nbnet_io_event {
  int      fd;
  uint32_t event;
  pth_t    tid;
};

size_t nbnet_events_size();
struct nbnet_io_event *nbnet_events(struct nbnet_io_event *);

#ifdef __cplusplus
}
#endif
