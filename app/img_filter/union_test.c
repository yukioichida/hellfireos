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

void populate_matrix(int8_t matrix[2][2]){
	matrix[1][1] = 2;
	matrix[1][0] = 2;
	matrix[0][0] = 2;
	matrix[0][1] = 2;
}

int main(){
	int k,i,j;

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

	int8_t var[5][5];
	printf("Size of matrix %ld\n", sizeof(var));

	int8_t m[2][2];

	printf("Original Matrix:\n");
	for (i=0; i < 2; i++){
		for(j=0; j < 2; j++){
			printf("%d\t", m[i][j]);
		}
	}
	printf("\n");
	populate_matrix(m);
	printf("Transformed Matrix:\n");
	for (i=0; i < 2; i++){
		for(j=0; j < 2; j++){
			printf("%d\t", m[i][j]);
		}
	}
	printf("\n");
}