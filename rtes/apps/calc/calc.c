#include <stdio.h>
#include <stdlib.h>
#include <asm/unistd.h>
#include <sys/syscall.h>

#define SCALE 1000

/*Wrapper for calc*/
float calc(float num1, float num2, char operation)
{
	float result = 0;
	long first = num1 * SCALE;
	long second = num2 * SCALE;
	int retval = syscall(__NR_calc, first, second, operation) ;
		
	switch(operation){

		case '+':
		case '-':
			result = ((float) retval) / SCALE;
			break;
		case '*':
			result = ((float) retval) / (SCALE * SCALE);
			break;
		case '/':
			result = retval;
			break;
		default:
			printf("Operation not supported\n");
			break;
	}
	return result;
}

void main(int argc, char** argv){

	float num1, num2;
	float result = 0;
	char op;
	
	num1 = atof(argv[1]);
	num2 = atof(argv[3]);
	op = *argv[2];
	result=calc(num1, num2, op);
	printf("Result=%f\n", result);
}
