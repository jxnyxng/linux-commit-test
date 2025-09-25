#include <stdio.h>
#include <stdlib.h>

int main(){

	int num1, num2;
	char op;

	scanf("%d %c %d", &num1, &op, &num2);

	if(op == '+'){
		printf("result : %d \n", num1 + num2);
	} else if(op == '-'){
		printf("result : %d \n", num1 - num2);
	} else if(op == 'x'){
		printf("result : %d \n", num1 * num2);
	}else{
		if (num2 == 0){
			printf("error!");
		} else{
			printf("result : %d \n", num1/num2);
		}
	}
	return 0;
}
