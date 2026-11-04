#include <stdio.h>
#include <pwd.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
int getfirstpart(char *buf);
int main(int argc, char *argv[]) {
	char builtin[5][1024] ;
	strcpy(builtin[0] , "echo");
	strcpy(builtin[1] , "exit");
	strcpy(builtin[2] , "type");
	strcpy(builtin[3] , "pwd");
	strcpy(builtin[4] , "cd");
	char* path = getenv("PATH");
	char paths[64][512] ;
	int path_count = 0 ;
	char *path_copy = strdup(path) ;
	char *token = strtok(path_copy , ":");
	while(token != NULL && path_count < 64){
		strncpy(paths[path_count],token,511);
		paths[path_count][511] = '\0';
		path_count++;
		token = strtok(NULL , ":");
	}
	free(path_copy);
	while(true){
		setbuf(stdout, NULL);
		printf("$ ");
		char command[1024];
		fgets(command , sizeof(command) , stdin);
		char rcommand[1024];
		int sizecommand = getfirstpart(command);
		strncpy(rcommand , command , sizecommand);
		rcommand[sizecommand] = '\0' ;
		command[strcspn(command, "\n")] = '\0' ; 
		char content[1024] = "" , *begin = strdup(command + sizecommand + 1) , *end = strdup(command + sizecommand + 1);
		while(end[0] != '\0'){
			begin = end ;
			if(begin[0] == ' ' && strlen(begin) != strlen(command + 5)){
				strcat(content, " ");
			}
			while(begin[0] == ' '){
				begin++ ;
			}
			if(begin[0] == '\''){
				end = begin + 1 ;
				while(end[0] != '\'' && end[0] != '\0' ){
					end++ ;
				}
				strncat(content, begin + 1 , strlen(begin) - strlen(end) - 1 );
				if(end[0] == '\'') end++;
			}
			else if(begin[0] == '\"'){
				end = begin + 1;
				while(end[0] != '\"' && end[0] != '\0'){
					if(end[0] == '\\'){
						strncat(content, begin + 1 , strlen(begin) - strlen(end) );
						end++;
						begin = end ;
					}
					end++;
				}
				strncat(content, begin + 1 , strlen(begin) - strlen(end) - 1);
				if(end[0] == '\"') end++;
			}
			else if(begin[0] == '\\'){
				end = begin + 1 ;
				if(end[0] != '\0'){
					end++;
				}
				strncat(content, begin + 1 , 1 );
			}
			else{
				end = begin ;
				while(end[0] != ' ' && end[0] != '\0' && end[0] != '\'' && end[0] != '\"' && end[0] != '\\'){
					end++;
				}
				strncat(content, begin, strlen(begin) - strlen(end) );
			}
		}
		if(strcmp(rcommand,"exit") == 0){
			return 0 ;
		}else if(strcmp(rcommand,"echo") == 0){
			if(strlen(command) > 5){
				printf("%s\n" , content);
			}
		}else if(strcmp(rcommand,"type") == 0){
			if(strlen(command) > 5){
				bool is_builtin = false , is_notbuiltin = false ;
				for(int i = 0 ; i < 5 && !is_builtin ; i++){
					if(strcmp(content,builtin[i]) == 0){
						printf("%s is a shell builtin\n" , builtin[i]);
						is_builtin = true ;
					}
				}
				for(int i = 0 ; i < path_count && !is_builtin ; ++i){
					char x[512];
					strncpy(x, paths[i] , 511 );
					x[511] = '\0';
					strcat(x,"/");
					strcat(x , content);
					if(access(x,X_OK) == 0){
						is_notbuiltin = true ;
						printf("%s is %s\n" , content , x);
						break;
					}
				}
				if(!is_builtin && !is_notbuiltin){
					printf("%s: not found\n" , content);
				}
			}
		}else if(strcmp(rcommand,"pwd") == 0){
			char cwd[1024];
			if(getcwd(cwd, sizeof(cwd))!= NULL){
				printf("%s\n" , cwd);
			}
		}else if(strcmp(rcommand,"cd") == 0){
			char *newpath = strdup(content);
			if(newpath[0] == '~'){
				char *home = getenv("HOME");
				char temp[1024];
				if(home != NULL){
					snprintf(temp , sizeof(temp),"%s%s" , home , content + 1);
					newpath = strdup(temp);
				}else{
					printf("HOME evnvironment is not set.\n");
				}
			}
			if(chdir(newpath) != 0){
				printf("cd: %s: No such file or directory\n" , newpath);
			}
		}else{
			char filename[512] = "" ;
			bool found = false ;
			for(int i = 0 ; i < path_count ; ++i){
				snprintf(filename, sizeof(filename) , "%s/%s" , paths[i] , rcommand );
				if(access(filename , X_OK) == 0){
					found = true;
					break;
				}
			}
			if (found){
				pid_t pid = fork();
				if( pid == 0 ){
					char *newargv[64];
					int argc = 1 ;
					newargv[0] = strndup(command, sizecommand);
					char *begin = strdup(command + sizecommand) , *end = strdup(command + sizecommand);
					while(end[0] != '\0'){
						begin = end ;
						while(begin[0] == ' '){
							begin++ ;
						}
						if(begin[0] == '\''){
							end = begin + 1 ;
							while(end[0] != '\'' && end[0] != '\0' ){
								end++ ;
							}
							newargv[argc]= strndup(begin + 1 , strlen(begin) - strlen(end) - 1 );
							if(end[0] == '\'') end++;
							argc++;
						}
						else if(begin[0] == '\"'){
							end = begin + 1;
							while(end[0] != '\"' && end[0] != '\0'){
								if(end[0] == '\\'){
									end++;
									char *temp = strndup(begin , strlen(begin) - strlen(end));
									char *temp2 = strdup(end);
									begin = strcat(temp, temp2);
								}
								end++;
							}
							newargv[argc] = strndup(begin + 1 , strlen(begin) - strlen(end) - 1);
							if(end[0] == '\"') end++;
							argc++;
						}
						else{
							end = begin ;
							while(end[0] != ' ' && end[0] != '\0'){
								if(end[0] == '\\'){
									char *temp = strndup(begin , strlen(begin) - strlen(end));
									char *temp2 = strdup(end + 1);
									begin = strcat(temp,temp2);
									if(end[1] != '\0'){
										end++;
									}
								}else if(end[0] == '\''){
									char *temp = strndup(begin , strlen(begin) - strlen(end));
									end++;
									char *temp2 = strdup(end) ;
									while(end[0] != '\''){
										end++;
									}
									char *temp3 = strndup(temp2,strlen(temp2) - strlen(end));
									strcat(temp,temp3);
									begin = strcat(temp,end+1);
								}else if(end[0] == '\"'){
									char *temp = strndup(begin , strlen(begin) - strlen(end)); // before "
									end++;
									char *temp2 = end ; // after "
									while(end[0] != '\"'){
										if(end[0] == '\\'){
											char *temp4 = strndup(temp2 , strlen(temp2) - strlen(end));
											temp2 = strcat(temp4 , end + 1);
											end++;
										}
										end++;
									}
									char *temp3 = strndup(temp2,strlen(temp2) - strlen(end)); // between "
									strcat(temp,temp3); // append before " to between "
									begin = strcat(temp,end+1); // append the rest to a parsed string
								}
								end++;
							}
							newargv[argc] = strndup(begin, strlen(begin) - strlen(end) );
							argc++;
						}
					}
					newargv[argc] = NULL ;
					execv(filename, newargv);
					perror("execv error");
					}else if(pid < 0){
						perror("fork error");
						exit(1);
					}else {
						wait(NULL);
					}
				}
			if(!found){
				printf("%s: command not found\n", rcommand);
				continue;
			}
		}
	}
	return 0;
}
int getfirstpart(char *buf){
	for(int i = 0 ; i < strlen(buf) ; i++){
		if(buf[i] == ' ' || buf[i] == '\0' || buf[i] == '\n'){
			return i ;
		}
	}
	return strlen(buf) ;
}
