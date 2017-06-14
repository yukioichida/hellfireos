#include <stdio.h>
#include <stdint.h>
#include <string.h>

struct Data {
	uint8_t i;
	uint32_t j;
};

union Package {
	struct Data data;
	int8_t values[32];
};

int main(){
	int k;

	union Package package;

	//package.data = data;
	package.data.i = 3;
	package.data.j = 3495;

	union Package output;

	printf("Value %d \n", package.values[1]);

	for (k = 0; k < 20; k++){
		output.values[k] = package.values[k];
	}

	uint32_t received_i = output.data.j;

	printf("Value of vector union: %d\n", received_i);

}