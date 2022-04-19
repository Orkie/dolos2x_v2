#include <dolos2x.h>
#include <gp2xregs.h>
#include <unistd.h>
#include <sys/time.h>

static struct timeval lastTickTime;
static volatile uint32_t rTCOUNT = 0;

static void TCOUNT_read(uint32_t addr, int bytes, void* ret) {  
  U32_VAL(ret) = rTCOUNT;
}

static void TCOUNT_write(uint32_t addr, int bytes, void* value) {
  rTCOUNT = U32_VAL(value); // TODO - simulate the problem with writing 0
}

static int init(add_mem_callback add_read_callback, add_mem_callback add_write_callback) {
  gettimeofday(&lastTickTime, NULL);
  add_read_callback(REG(TCOUNT), TCOUNT_read);
  add_write_callback(REG(TCOUNT), TCOUNT_write);

  return 0;
}

static void tick(void) {
  // TODO -only if timer is enabled
  time_t lastSec = lastTickTime.tv_sec;
  suseconds_t lastUsec = lastTickTime.tv_usec;
  gettimeofday(&lastTickTime, NULL);
  
  unsigned long long elapsedNsecs = ((lastTickTime.tv_sec - lastSec) * 1000000000) + ((lastTickTime.tv_usec - lastUsec) * 1000);
  unsigned long long elapsedTicks = (elapsedNsecs / 135);
  rTCOUNT += elapsedTicks;
}

dolos_peripheral peri_timer = {
  .name = "Timer",
  .init = &init,
  .tick = &tick
};

