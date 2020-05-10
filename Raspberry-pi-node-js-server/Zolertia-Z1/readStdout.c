#include <stdio.h>

int main(void)
{
	char buffer[BUFSIZ];
	char line[BUFSIZ];
	int pointer = 0;
	while(fgets(buffer, BUFSIZ, stdin) != NULL)
	{/*
		if(buffer[0] != '\n')
		{
			line[pointer] = buffer[0];
			pointer++;
		}
		else
		{
			//pointer++;
			//line[pointer] = '\n';
		}
		//printf("GOOD DAY: %s\n", line);
		printf("GOOD DAY!\n");
		*/
		printf("GOOD DAY!\n");
	}
}
