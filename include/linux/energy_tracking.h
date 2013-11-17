#ifndef __ENERGY_TRACKING_H_
#define __ENERGY_TRACKING_H_

int get_cpu_energy(unsigned int freq);
void energy_accounting(struct task_struct* prev);

#endif
