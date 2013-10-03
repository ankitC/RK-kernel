#ifndef HR_TIMER_FUNC_H_
#define HR_TIMER_FUNC_H_

enum hrtimer_restart my_hrtimer_callback( struct hrtimer *timer );
void init_hrtimer( struct reserve_obj *res_p);


void cleanup_hrtimer(struct hrtimer *hr_timer );

#endif 
