#include <linux/module.h>   /* Needed by all modules */
#include <linux/kernel.h>   /* Needed for KERN_INFO */
#include <linux/init.h>     /* Needed for the macros */
#include <linux/jiffies.h>
#include <linux/time.h>
#define MAXRUNS 200

#include <linux/hrtimer.h>


static volatile int runcount = 0;

//~ static struct timer_list my_timer;
static unsigned long period_ms;
static unsigned long period_ns;
static ktime_t ktime_period_ns;
static struct hrtimer my_hrtimer;


//~ static void testjiffy_timer_function(unsigned long data)
static enum hrtimer_restart testjiffy_timer_function(struct hrtimer *timer)
{
  int tdelay = 100;
  unsigned long tjnow;
  ktime_t kt_now;
  int ret_overrun;

  runcount++;
  if (runcount == 5) {
    while (tdelay > 0) { tdelay--; } // small delay
  }

  printk(KERN_INFO
    " %s: runcount %d \n",
    __func__, runcount);

  if (runcount < MAXRUNS) {
    tjnow = jiffies;
    kt_now = hrtimer_cb_get_time(&my_hrtimer);
    ret_overrun = hrtimer_forward(&my_hrtimer, kt_now, ktime_period_ns);
    printk(KERN_INFO
      " testjiffy jiffies %lu ; ret: %d ; ktnsec: %lld \n",
      tjnow, ret_overrun, ktime_to_ns(kt_now));
    return HRTIMER_RESTART;
  }
  else return HRTIMER_NORESTART;
}


static int __init testjiffy_init(void)
{
  struct timespec tp_hr_res;
  period_ms = 1000/HZ;
  hrtimer_get_res(CLOCK_MONOTONIC, &tp_hr_res);
  printk(KERN_INFO
    "Init testjiffy: %d ; HZ: %d ; 1/HZ (ms): %ld ; hrres: %lld.%.9ld\n",
               runcount,      HZ,        period_ms, (long long)tp_hr_res.tv_sec, tp_hr_res.tv_nsec );

  hrtimer_init(&my_hrtimer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
  my_hrtimer.function = &testjiffy_timer_function;
  period_ns = period_ms*( (unsigned long)1E6L );
  ktime_period_ns = ktime_set(0,period_ns);
  hrtimer_start(&my_hrtimer, ktime_period_ns, HRTIMER_MODE_REL);

  return 0;
}

static void __exit testjiffy_exit(void)
{
  int ret_cancel = 0;
  while( hrtimer_callback_running(&my_hrtimer) ) {
    ret_cancel++;
  }
  if (ret_cancel != 0) {
    printk(KERN_INFO " testjiffy Waited for hrtimer callback to finish (%d)\n", ret_cancel);
  }
  if (hrtimer_active(&my_hrtimer) != 0) {
    ret_cancel = hrtimer_cancel(&my_hrtimer);
    printk(KERN_INFO " testjiffy active hrtimer cancelled: %d (%d)\n", ret_cancel, runcount);
  }
  if (hrtimer_is_queued(&my_hrtimer) != 0) {
    ret_cancel = hrtimer_cancel(&my_hrtimer);
    printk(KERN_INFO " testjiffy queued hrtimer cancelled: %d (%d)\n", ret_cancel, runcount);
  }
  printk(KERN_INFO "Exit testjiffy\n");
}

module_init(testjiffy_init);
module_exit(testjiffy_exit);

MODULE_LICENSE("GPL");
