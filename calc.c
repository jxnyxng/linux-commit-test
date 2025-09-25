#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]){

	int num1 = atoi(argv[1]);
	char op = argv[2][0];
	int num2 = atoi(argv[3]);

	if(op == '+'){
		printf("result : %d \n", num1 + num2);
	} else if(op == '-'){
		printf("result : %d \n", num1 - num2);
	} else if(op == 'x'){
		printf("result : %d \n", num1 * num2);
	}else{
		if (num2 == 0){
			print("error!");
		} else{
			print("result : %d \n", num1/num2);
		}
	}
	return 0;
}
