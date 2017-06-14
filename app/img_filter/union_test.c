#include <stdio.h>
#include <stdint.h>
#include <string.h>

struct Data {
	uint16_t i;
	uint16_t j;
};

union Package {
	struct Data data;
	uint8_t values[32];
};

int main(){

	struct Data data;
	data.i = 43000;
	data.j = 45;

	union Package package;

	union Package output;

	package.data = data;

	int k;

	for (k = 0; k < 20; k++){
		output.values[k] = package.values[k];
	}

	uint16_t received_i = output.data.i;

	printf("Value of vector union: %d\n", received_i);

}