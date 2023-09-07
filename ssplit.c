#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <wordexp.h>
#include <signal.h>
#include <fcntl.h>

#define PIPE_IN 1
#define PIPE_OUT 0

typedef struct Split_Options Split_Options;
struct Split_Options{
	char delimiter;
	char EOL;
};

typedef struct Split_Target Split_Target;
struct Split_Target{
	int pid;
	int pipe_in;
	int pipe_out;
	int dead;
};

void print_help(){
	printf("ssplit reads stdin line by line, splits each line based on a\n delimiter and sends the output to different target processes.\n\n");
	printf("usage: ssplit [options] \"[target] [target options]\" \"[...]\"\n");
	printf("targets including their arguments must be enclosed in quotes (\")\n");
	printf("options:\n");
	printf("  set delimiter	: -d [delimiter charachter] (default: ' ')\n");
	printf("  set EOL	: -e [EOL charachter] (default: '\\n')\n");
}

void start_target(Split_Target* target, char* target_cmd){
	if(strlen(target_cmd) == 0) return;
	int pipefd_in[2];
	if(pipe(pipefd_in) < 0){
		fprintf(stderr,"Error while trying to create pipe in: %s\n",strerror(errno));
		exit(1);
	}

	int pipefd_out[2];
	if(pipe(pipefd_out) < 0){
		fprintf(stderr,"Error while trying to create pipe out: %s\n",strerror(errno));
		exit(1);
	}

	pid_t child = fork();
	if (child < 0){
		fprintf(stderr,"Error while trying to create child process: %s\n",strerror(errno));
		exit(1);
	}

	if(child == 0){ // if this process is the child
		// redirect child stdin and stdout
		if(dup2(pipefd_in[PIPE_OUT], STDIN_FILENO) < 0){
			fprintf(stderr,"Could not attach pipe output to stdin of process: %s\n",strerror(errno));
			exit(1);
		}
		if(dup2(pipefd_out[PIPE_IN], STDOUT_FILENO) < 0){
			fprintf(stderr,"Could not attach pipe output to stdin of process: %s\n",strerror(errno));
			exit(1);
		}

		wordexp_t parsed_cmd;
		int ret = wordexp(target_cmd, &parsed_cmd, 0); // split target command args
		if(ret < 0){
			fprintf(stderr,"Error occured while parsing target cmd '%s': %s\n",target_cmd,strerror(errno));
			exit(1);
		}
		ret = execv(parsed_cmd.we_wordv[0],parsed_cmd.we_wordv);
		if(ret < 0){
			fprintf(stderr,"Error occured while running target cmd '%s': %s\n",target_cmd,strerror(errno));
			exit(1);
		}
		printf("child proc return with: %d\n",ret);
		exit(ret); // exit when any of the target processes is done
	}
	printf("child proc: %d created\n",child);


	target->pipe_in = pipefd_in[PIPE_IN];
	fcntl(target->pipe_in, F_SETFL, O_NONBLOCK);
	target->pipe_out = pipefd_out[PIPE_OUT];
	fcntl(target->pipe_out, F_SETFL, O_NONBLOCK);
	target->pid = child;
}

#define MAX_TARGETS 32

Split_Target targets[MAX_TARGETS] = {0};
int target_count = 0;

void handle_sigpipe(int sig)
{
	for(size_t i = 0; i < target_count; i++){
		kill(targets[i].pid,SIGKILL);	
	}
	close(STDIN_FILENO);
	exit(1);
}

void write_to_target(Split_Target* target, char* s, size_t n){
	if(target->pid == 0) return;
	int ret = write(target->pipe_in,s,n);
	ret = write(target->pipe_in,"\n",1);
	target->dead = ret < 0;
	if(target->dead){
		handle_sigpipe(SIGPIPE);
	}

	// flush target stdout
	char out[1024];
	int read_bytes = 0;
	read_bytes = read(target->pipe_out,out,1024);
	while(read_bytes > 0){
		write(STDOUT_FILENO,out,read_bytes);
		read_bytes = read(target->pipe_out,out,1024);
	}

}

int main(int argc, char** argv){
	signal(SIGPIPE,handle_sigpipe);
	if(argc < 2) {
		print_help();
		return 1;
	}
	Split_Options options = {
		.delimiter=' ',
		.EOL='\n'
	};
	
	Split_Target* current_target = targets;

	for(size_t i = 1; i < argc; i++){
		if(argv[i][0] == '-'){ // check if arg is option
			if(i+1 >= argc) {
				print_help();
				return 1;
			}
			switch(argv[i][1]){ // handle option
				case 'd':{ // set delimiter
					i++;
					options.delimiter = argv[i][0];
				} break;
				case 'e':{ // set EOL
					i++;
					options.EOL = argv[i][0];
				} break;
				default:{
					print_help();
					return 1;
				} break;
			}
		}else{
			if(current_target >= targets + MAX_TARGETS){
				printf("Error: target limit reached\n");
				exit(1);
			}
			start_target(current_target, argv[i]);
			current_target++;
			target_count++;
		}
	}
	
	current_target = targets;

	size_t line_length = 0;
	char* line = NULL;

	char buf[2048];
	size_t buf_i = 0;

	while(getdelim(&line,&line_length,options.EOL,stdin) >= 0){
		char* token = strtok(line, " \n");
		while(token){
			write_to_target(current_target, token, strlen(token));
			current_target++;
			if(current_target->pid == 0 || current_target >= targets + MAX_TARGETS){
				break;
			}
			token = strtok(NULL, " \n");
		}
		current_target = targets;
		
		// check if any targets are still alive
		int dead_targets = 0;
		while(current_target->pid != 0 || current_target < targets + MAX_TARGETS){
			if(current_target->dead){ dead_targets++; }
			current_target++;
		}

		current_target = targets;

	}

	return 0;
}
