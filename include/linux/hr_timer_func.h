#ifndef HR_TIMER_FUNC_H_
#define HR_TIMER_FUNC_H_

enum hrtimer_restart T_timer_callback( struct hrtimer *timer );
void init_hrtimer( struct reserve_obj *res_p);
enum hrtimer_restart C_timer_callback( struct hrtimer *timer);
void cleanup_hrtimer(struct hrtimer *hr_timer );

#endif 
