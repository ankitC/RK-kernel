#ifndef HR_TIMER_FUNC_H_
#define HR_TIMER_FUNC_H_

enum hrtimer_restart my_hrtimer_callback( struct hrtimer *timer );
struct hrtimer init_hrtimer( struct timespec T);


void cleanup_hrtimer(struct hrtimer hr_timer );

#endif 
