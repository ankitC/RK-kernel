#include <stdio.h>
#include <stdlib.h>

void end_job_t(){

	int x = 0;
	int i = 0;
	while (1){
		i++;
		sleep(2);
		end_job();
		x++;
		printf("Value of x on %dth iteration = %d\n", i, x);
	}

}

int main(){

	end_job_t();
	return 0;

}
