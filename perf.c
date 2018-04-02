#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <regex.h>
#include <sys/stat.h>
#include <fcntl.h>


typedef struct{
   char name[40];
   double time;
   double percent;
}Sys;
Sys Syscall[512];
int num;
int fileds[2];

void child(int argc, char* argv[]){
     char * p_argv[12];
     p_argv[0] = "strace";
     p_argv[1] = "-T"; 
     if(argc <= 1){
	 printf("Please type in more commands to run perf.\n");
	 exit(1);
     }
     for(int i = 2; i <= argc; i++){
	     p_argv[i] = argv[i-1];
     }
     /* for(int de = 0; de < argc+2; de++){
      * printf("%s", p_argv[de]);
      }*/
     // printf("\n");
     p_argv[argc + 1] = NULL; 
     dup2(fileds[1], 2);
     int rel = open("/dev/null", O_WRONLY | O_APPEND);
     dup2(rel, 1);
     //char *envp[] = {"PATH=/bin", NULL};
     execvp(p_argv[0], p_argv);
     //printf("An error occurred when execving.\n");
     perror("execvp");
     return;     
}

void parent(){
       dup2(fileds[0], 0);
       char buf[1024] = {0};
       while(fgets(buf, 1024, stdin)!= NULL){
         if(!strncmp(buf, "+++", 3)){
	    break;
	 }
//	 printf("%s\n", buf);
	 char temp_name[40] = {0};
	 char temp_time[40] = {0};
	 int status, chk;
	 int cflags = REG_EXTENDED;
	 regmatch_t pmatch[1];
	 const size_t nmatch = 1;
	 regex_t reg;
         const char *pattern1 = "[A-Za-z,_]+([0-9]+)?";
         const char *pattern = "\\<([0-9]+)\\.([0-9]+)";
         regcomp(&reg, pattern1, cflags);
	 status = regexec(&reg, buf, nmatch, pmatch, 0);
	 if(status == REG_NOMATCH)
               printf("No match\n");
         else if(status == 0){
              for(chk = pmatch[0].rm_so; chk < pmatch[0].rm_eo; ++chk){
                 temp_name[chk - pmatch[0].rm_so] = buf[chk]; 
	      }
	 }
         regfree(&reg);
         regcomp(&reg, pattern, cflags);
         status = regexec(&reg, buf, nmatch, pmatch, 0);
	 if(status == REG_NOMATCH)
            printf("No match\n");
	 else if(status == 0){
	       for(chk = pmatch[0].rm_so; chk < pmatch[0].rm_eo; ++chk){
		       temp_time[chk - pmatch[0].rm_so] = buf[chk];
		      // putchar(buf[chk]);
               }
	       //printf("\n");
	 }
	 regfree(&reg);


          int flag = 0;
          for(int i = 0; i < num; i++){
            if(!strcmp(temp_name, Syscall[i].name)){
	       Syscall[i].time += atof(temp_time);
	       flag = 1;
	       break;
	    }
          }
          if(!flag){
	      strcpy(Syscall[num].name, temp_name);
	      Syscall[num].time = atof(temp_time);
	   //   printf("%s  %lf\n", Syscall[num].name, Syscall[num].time);
	      num++;
	      }
        }   
}

int main(int argc, char *argv[]) {
       pid_t pid;
	int ret = pipe(fileds);
       if(ret != 0){
          perror("pipe error\n");
          return 1;
       }
       pid = fork();
       if(pid < 0){ //invalid
          printf("An error occurred when forking.\n");
       }
       else if(pid == 0){ //child
	  close(fileds[0]);
          child(argc, argv);
       }
       else{ //father
	      close(fileds[1]);
   	      num = 0;
	      parent();
	      num -= 1;
       }
       double sum = 0.0;
       for(int s = 0; s <= num; s++){
          sum += Syscall[s].time;
       }
       for(int s = 0; s <= num; s++){
          Syscall[s].percent = Syscall[s].time * (double)100/sum;
       }
        printf("Sysname                        Time(%%)\n");
	printf("--------------------------------------\n");
       for(int s = 0; s <= num; s++){
        printf("%-18s               %.2lf\n", Syscall[s].name, Syscall[s].percent);
       }
       return 0;
}
