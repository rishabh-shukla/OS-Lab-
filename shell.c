#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/stat.h>
#include <wchar.h>
#include <math.h>
#include <fcntl.h>
#include <signal.h>

char buffer[1024];
int cmd_count = 0;
char input;
int count_buffer = 0;
char *cmd_array[64];
char cwd[1024];
int flag_input = 0;
int flag_output = 0;
char *file_input;
char *file_output;
int piping = 0;
int isBckg = 0;
char cwd[1024];


struct commands
{
	char *cmd_array[64];
	int cmd_count;
}command[50];

void initialize()
{
	input = '\0';
	int i=0;
	int j=0;
	for(i=0;i<=piping;i++)
	{
		for(j=0;j<=command[i].cmd_count;j++)
		{
			command[i].cmd_array[j] = NULL;
		}
	}
	piping = 0;
	file_input = NULL;
	file_output = NULL;
	flag_input = 0;
	isBckg = 0;
	flag_output = 0;
	cmd_count = 0;
	while(count_buffer >= 0)
	{
		buffer[count_buffer] = '\0';
		count_buffer--;
	}
	count_buffer = 0;
}

void create_process (int in, int out, struct commands *cmd)
{
	pid_t pid;
	if ((pid = fork ()) == 0)
	{
		if (in != 0)
  		{
			dup2 (in, 0);
			close (in);
  		}
		if (out != 1)
		{
			dup2 (out, 1);
			close (out);
		}

		if(execvp (cmd->cmd_array[0], (char * const *)cmd->cmd_array ) < 0)
		{
			printf("An error occured while executing the command \n");
			exit(1);
		}
	}
}

void pipe_run (int n, struct commands *cmd)
{
	if(strcmp(cmd[0].cmd_array[0]	, "cd")==0)
	{
		if(!cmd[0].cmd_array[1])
		{
			if(chdir(getenv("HOME"))!=0);
		}
		else
		{
			if(chdir(cmd[0].cmd_array[1])!=0)
			{
				printf("Invalid Path");
			}
		}
	}
	else if(strcmp(cmd[0].cmd_array[0]	, "mkdir")==0)
	{
		if(mkdir(cmd[0].cmd_array[1], 0700)!=0)
		{
			printf("Invalid Name of Directory or Directory already exist \n");
		}
	}
	else if(strcmp(cmd[0].cmd_array[0]	, "rmdir")==0)
	{
		if(rmdir(cmd[0].cmd_array[1])!=0)
		{
			printf("Invalid Name of Directory or Directory already exist \n");
		}
	}
	else if(strcmp(cmd[0].cmd_array[0]	, "exit")==0)
	{
		exit(0);
	}
	else
	{
		pid_t pid;
  		int i;
  		int ini, out;
		int  status;
  		int in, fd [2];  		
  		in = 0;  		
  		for (i = 0; i < n - 1; ++i)
    	{
      		pipe (fd);
      		create_process (in, fd [1], cmd + i);      		
      		close (fd [1]);     		
      		in = fd [0];
    	}
  		
  		if((pid = fork()) == 0)
  		{
  			if (in != 0){
    			dup2 (in, 0);
    			close(in);
			}
			if(flag_output == 2)
			{
        		out = open(file_output, O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
        		dup2(out, 1);
        		close(out);
      		}
			if(flag_output == 4)
			{
				out = open(file_output, O_RDWR | O_APPEND | O_CREAT, 0777);
				dup2(out, 1);
				close(out);
			}
    		if(flag_input == 2)
    		{
        		ini = open(file_input, O_RDONLY);
        		dup2(ini, 0);
        		close(ini);
    		}

   			if(execvp (cmd [i].cmd_array[0], (char * const *)cmd [i].cmd_array )<0)
   			{
   				printf("An error occured while executing the command \n");
        		exit(1);
   			}
		}
		else
		{
			if(isBckg==0)
				while (wait(&status) != pid);
		}
	}
}

void break_command(char *cmd)
{
	char *var;
	var = strtok(cmd, " ");
	while( var != NULL )
   	{
   	   if(flag_output == 0 && strcmp(var, "|") != 0 && strcmp(var, "<") != 0 && strcmp(var, "&") != 0)
   	   {
			command[piping].cmd_array[cmd_count] = var;
			cmd_count++;
			command[piping].cmd_array[cmd_count] = NULL;
   	   }
   	   var = strtok(NULL, " ");


   	   if(flag_output==1)
   	   {
   	   		file_output = var;
   	   		flag_output = 2;
   	   }
   	   if(flag_output==3)
   	   {
			file_output = var;
			flag_output = 4;
		}
   	   if(flag_input==1)
   	   {
   	   		file_input = var;
   	   		flag_input = 2;
   	   }

   	   if(var)
   	   {
			if(strcmp(var, ">") == 0)
			{
				flag_output = 1;
			}
			if(strcmp(var, ">>") == 0)
			{
				flag_output = 3;
			}
			if(strcmp(var, "<") == 0 && flag_input == 0)
			{
				flag_input = 1;
			}
			if(strcmp(var, "&") == 0)
			{
				isBckg = 1;
				command[piping].cmd_array[cmd_count] = NULL;
				command[piping].cmd_count = cmd_count;
				cmd_count = 0;
				piping++;
			}

			if(strcmp(var, "|") == 0)
			{
				command[piping].cmd_array[cmd_count] = NULL;
				command[piping].cmd_count = cmd_count;
				cmd_count = 0;
				piping++;
			}
		}

   	}
   	pipe_run(piping+1, command);
}



int main()
{

	system("clear");

	while(1){
		printf("%s $ ", getcwd(cwd, sizeof(cwd)));

		initialize();
	while(input!='\n'){

		input = getchar();
		if(input=='\n') break;

		buffer[count_buffer] = input;
		count_buffer++;
	}
	if(buffer[0]!='\0')
        break_command(buffer);

	}
	return 0;
}
