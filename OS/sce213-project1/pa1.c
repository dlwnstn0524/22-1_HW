/**********************************************************************
 * Copyright (c) 2021
 *  Sang-Hoon Kim <sanghoonkim@ajou.ac.kr>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTIABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 **********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <unistd.h>
#include <errno.h>

#include <string.h>

#include "types.h"
#include "list_head.h"
#include "parser.h"
#include <sys/wait.h>
#include <signal.h>


/***********************************************************************
 * struct list_head history
 *
 * DESCRIPTION
 *   Use this list_head to store unlimited command history.
 */
LIST_HEAD(history);
int is_forked = 0;
int i = 0;
int flag = 0;
struct entry {
		struct list_head list;
		int index; 
		char *command;
};
/***********************************************************************
 * append_history()
 *
 * DESCRIPTION
 *   Append @command into the history. The appended command can be later
 *   recalled with "!" built-in command
 */
static void append_history(char * const command)
{
	
	struct entry *entry = (struct entry*)malloc(sizeof(struct entry));
	entry->index = i;
	entry->command = (char*)malloc(sizeof(char)*strlen(command)+1);
	strcpy(entry->command, command);
	i++;
	list_add_tail(&entry->list, &history);
}

/***********************************************************************
 * run_command()
 *
 * DESCRIPTION
 *   Implement the specified shell features here using the parsed
 *   command tokens.
 *
 * RETURN VALUE
 *   Return 1 on successful command execution
 *   Return 0 when user inputs "exit"
 *   Return <0 on error
 */
static int run_command(int nr_tokens, char *tokens[])
{
	int is_piped = 0;
	
	for(int j = 0 ; j < nr_tokens ; j++){
		if(strncmp(tokens[j], "|", strlen("|"))== 0){
			is_piped = 1;
			flag = j;
			break;
		}
	}

	if(is_piped){
		int fd[2];
		// char *cmd1[flag] = { NULL };
		// char * cmd2[nr_tokens - flag - 1] = {NULL};
		char *cmd1[100] = { NULL };
		char *cmd2[100] = {NULL};
		int status1;

		int piped = pipe(fd);
		for (int n = 0; n < flag; n++)
		{
			// cmd1[n] = (char*)malloc(sizeof(char*)*strlen(tokens[n]));
			// strncpy(cmd1[n], tokens[n], strlen(tokens[n]));
			cmd1[n] = tokens[n];
		}

		for (int n = flag + 1; n < nr_tokens; n++)
		{
			// cmd2[n - flag - 1] = (char*)malloc(sizeof(char)*strlen(tokens[n]));
			// strncpy(cmd2[n-flag-1], tokens[n], strlen(tokens[n]));
			cmd2[n - flag - 1] = tokens[n];
		}

		if (piped == -1)
		{
			fprintf(stderr, "pipe error: %s\n", strerror(errno));
			exit(EXIT_FAILURE);
			return -1;
		}

		pid_t pid1 = fork();
		if (pid1 == -1)
		{
			fprintf(stderr, "fork error: %s\n", strerror(errno));
			exit(EXIT_FAILURE);
			return -1;
		}
		else if (pid1 == 0)
		{ // 1st-child process
			dup2(fd[1], 1);
			close(fd[0]);
			is_forked = 1;
			run_command(flag, cmd1);
			exit(127);
		}

		else
		{ // parent process
			 pid1 = fork();

			if (pid1 == 0)
			{ // 2nd-child process
				wait(&status1);
				dup2(fd[0], 0);

				close(fd[1]);
				is_forked = 1;
				run_command(nr_tokens - flag - 1, cmd2);
				exit(127);
			}

			wait(&status1);
			close(fd[0]);
			close(fd[1]);
			is_forked = 0;
			return 1;
		}
	}
	
	if (strcmp(tokens[0], "exit") == 0) return 0;
	else if (strncmp(tokens[0], "cd", strlen("cd")) == 0){ // cd 명령어 입력
		if(nr_tokens == 1){
			chdir(getenv("HOME"));
		}
		else if(strncmp(tokens[1], "~", strlen("~")) == 0){
			chdir(getenv("HOME"));
		}
		else{
			int check = chdir(tokens[1]);
			if(check == -1){
				fprintf(stderr, "cd: no such file or directory: %s\n", tokens[1]);
			}
		}
		return 1;
	}
	else if (strncmp(tokens[0], "!", strlen("!")) == 0){  //! index 명령어 입력
		struct entry *a;

		list_for_each_entry(a, &history, list){
			if(a->index == atoi(tokens[1])) {				
				char *cmd = (char*)malloc(sizeof(char)*(strlen(a->command)-1));
				strncpy(cmd, a->command, strlen(a->command)-1);
				char *tokens_1[MAX_NR_TOKENS] = { NULL };
				int nr_tokens_1 = 0;
				if (parse_command(cmd, &nr_tokens_1, tokens_1) == 0){
					return 1;
				}
				return run_command(nr_tokens_1, tokens_1);
			}
		}
		return 1;
	}
	else if (strcmp(tokens[0], "history") == 0){ // history 명령어 입력
		struct entry *a;
		list_for_each_entry(a, &history, list){
			fprintf(stderr,"%2d: %s", a->index, a->command);
		}
		return 1;
	}
	else{
		int status;
		if(is_forked){
			execvp(tokens[0], tokens);
		}
		else{
			pid_t pid = fork();
		
			if(pid == 0){
				//printf("a : %s\nb:%s\n",tokens[0],tokens[1]);
				execvp(tokens[0], tokens);
				fprintf(stderr, "Unable to execute %s\n", tokens[0]);
				// for(int n =0 ; n < nr_tokens ; n++){
				// 		fprintf(stderr, "%s\n", tokens[n]);
				// }
				//if(m == -1){
					
				// 	fprintf(stderr, "Unable to execute %s\n", tokens[0]);
				// 	exit(127);
				// 	return -1;
				// }
				return 1;
			}
			else if (pid > 0){
				pid_t waitPid = wait(&status);
				if(waitPid == -1){
					fprintf(stderr, "Unable to execute %s\n", tokens[0]);
					exit(127);
					return -1;
				}
			}
		}
	}
	return 1;
}

/***********************************************************************
 * initialize()
 *
 * DESCRIPTION
 *   Call-back function for your own initialization code. It is OK to
 *   leave blank if you don't need any initialization.
 *
 * RETURN VALUE
 *   Return 0 on successful initialization.
 *   Return other value on error, which leads the program to exit.
 */
static int initialize(int argc, char * const argv[])
{

	return 0;
}


/***********************************************************************
 * finalize()
 *
 * DESCRIPTION
 *   Callback function for finalizing your code. Like @initialize(),
 *   you may leave this function blank.
 */
static void finalize(int argc, char * const argv[])
{

}


/*====================================================================*/
/*          ****** DO NOT MODIFY ANYTHING BELOW THIS LINE ******      */
/*          ****** BUT YOU MAY CALL SOME IF YOU WANT TO.. ******      */
static int __process_command(char * command)
{
	char *tokens[MAX_NR_TOKENS] = { NULL };
	int nr_tokens = 0;

	if (parse_command(command, &nr_tokens, tokens) == 0)
		return 1;

	return run_command(nr_tokens, tokens);
}

static bool __verbose = true;
static const char *__color_start = "[0;31;40m";
static const char *__color_end = "[0m";

static void __print_prompt(void)
{
	char *prompt = "$";
	if (!__verbose) return;

	fprintf(stderr, "%s%s%s ", __color_start, prompt, __color_end);
}

/***********************************************************************
 * main() of this program.
 */
int main(int argc, char * const argv[])
{
	char command[MAX_COMMAND_LEN] = { '\0' };
	int ret = 0;
	int opt;

	while ((opt = getopt(argc, argv, "qm")) != -1) {
		switch (opt) {
		case 'q':
			__verbose = false;
			break;
		case 'm':
			__color_start = __color_end = "\0";
			break;
		}
	}

	if ((ret = initialize(argc, argv))) return EXIT_FAILURE;

	/**
	 * Make stdin unbuffered to prevent ghost (buffered) inputs during
	 * abnormal exit after fork()
	 */
	setvbuf(stdin, NULL, _IONBF, 0);

	while (true) {
		__print_prompt();

		if (!fgets(command, sizeof(command), stdin)) break;

		append_history(command);
		ret = __process_command(command);

		if (!ret) break;
	}

	finalize(argc, argv);

	return EXIT_SUCCESS;
}