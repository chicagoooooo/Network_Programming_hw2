#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#include <signal.h>

char names[64][64];
int max_size = 64;
int socket_fds[64]={0};
void *server_thread(void *fd);
int auth(char *buf,int fd);
void list_all_people(int fd);
void send_to_all(char *buf);
void print_manual(int fd);
void recv_invite(int fd,char *buf);
void recv_yes(int fd,char *buf);
void recv_no(int fd,char *buf);
void recv_position(int fd,char *buf);
void recv_LOSE(int fd,char *buf);
void recv_FAIR(int fd,char *buf);
void recv_leave(int fd,char *buf);
void send_private_mag(int fd,char *buf);
int play_state[64]={0};
int main(){
    int listenfd,socketfd;
    if((listenfd = socket(PF_INET,SOCK_STREAM,0))<0){
        printf("Socket Create error!\n");
        exit(3);
    }

	struct sockaddr_in serv_addr;
    struct sockaddr_in cli_addr;
    socklen_t len;
	serv_addr.sin_family = PF_INET;
	serv_addr.sin_port = htons(10003);
	serv_addr.sin_addr.s_addr=htonl(INADDR_ANY);

	if (bind(listenfd,(struct sockaddr*)&serv_addr,sizeof(serv_addr)) == -1){
		perror("Bind Failed\n");
		exit(-1);
	}

	if (listen(listenfd,64) == -1){
		perror("Listen Failed\n");
		exit(-1);
	}

	printf("Server Initial Successful!\n");
	while(1){
        len=sizeof(cli_addr);
		if((socketfd = accept(listenfd,(struct sockaddr*)&cli_addr,&len))<0){
            printf("accept Client error\n");
            continue;
        }
        int i;
		for (i = 0;i < max_size;i++){
			if (socket_fds[i] == 0){
				socket_fds[i] = socketfd;
				pthread_t tid;
				pthread_create(&tid,NULL,server_thread,&socketfd);
				break;
			}
			if (max_size-1 == i){
				send(socketfd,"Sorry, the room is full!\0",strlen("Sorry, the room is full!\0"),0);
				close(socketfd);
			}
		}
	}
}


void *server_thread(void *a){
    int fd= *(int*)a;
    printf("pthread = %d\n",fd);
    char buf[1024];
    while(1){
        /*Login account*/
        send(fd,"authenticate\0",strlen("authenticate\0"),0);
        recv(fd,buf,1024,0);
        // printf("server recv = %s\n",buf);
        int success_login=auth(buf,fd);
        if(!success_login){
            send(fd,"LOGIN FAIL\n",strlen("LOGIN FAIL\n"),0);
            fflush(stdout);
            printf("USER LOGIN FAIL\n");
            fflush(stdout);
            pthread_exit((void*)-1);
        }else{
            print_manual(fd);
            break;
        }
    }
    while(1){
        if(recv(fd,buf,1024,0)<=0){
            int i;
            for(i=0;i<max_size;i++){
                if(fd==socket_fds[i]){
                    socket_fds[i]=0;
                }
            }
            fflush(stdout);
            printf("fd = %d,name = %s is leave\n" ,fd,names[fd]);
            fflush(stdout);
            pthread_exit((void*)1);
        }
        if(strncmp(buf,"l",1)==0){
            list_all_people(fd);
        }else if(buf[0]=='@'){
            recv_invite(fd,buf);
        }else if(strncmp(buf,"yes ",4)==0){
            recv_yes(fd,buf);
        }else if(strncmp(buf,"no ",3)==0){
            recv_no(fd,buf);
        }else if(buf[0]=='#'){
            recv_position(fd,buf);
        }else if(strncmp(buf,"LOSE",4)==0){
            recv_LOSE(fd,buf);
        }else if(strncmp(buf,"FAIR",4)==0){
            recv_FAIR(fd,buf);
        }else if(strncmp(buf,"LEAVE",5)==0){
            recv_leave(fd,buf);
        }else if(strncmp(buf,"all",3)==0){
            fflush(stdout);
            printf("[%d] [%s] send msg to all user\n",fd,names[fd]);
            fflush(stdout);
            send_to_all(buf);
        }else if(buf[0]=='/'){
            send_private_mag(fd,buf);
        }
    }
    
}

int auth(char *buf,int fd){
    char name[32];
    char passwd[32];
    char name_and_passwd[64]={0};
    // printf("server recv : %s\n",buf);
    char *ptr = buf;
    char *qtr=NULL;
    while(1){
        if(*ptr==':'){
            *ptr='\0';
            qtr=ptr;
        }
        if(*ptr=='$'){
            *ptr='\0';
            break;
        }
        ptr++;
    }
    strcpy(name,buf);
    strcpy(passwd,qtr+1);
    printf("name : %s\n",name);
    printf("passwd : %s\n",passwd);

    strcat(name_and_passwd,name);
    strcat(name_and_passwd,":");
    strcat(name_and_passwd,passwd);
    

    printf("name_and_passwd : %s\n",name_and_passwd);
    
	FILE *fp;
	char tmp[64];
	fp=fopen("passwd","r");
	while(fscanf(fp,"%s",tmp)!=EOF)
	{
        // printf("%s\n",tmp);
		if(strcmp(tmp,name_and_passwd)==0){
            strcpy(names[fd],name);
            return 1;
        }

	}
    for(int i=0;i<max_size;i++){
        if(socket_fds[i]==fd){
            socket_fds[i]=0;
        }
    }
	return 0;
}


void list_all_people(int fd){
    send(fd,"----------now online user----------\n",strlen("----------now online user----------\n"),0);
    send(fd,"[name]\t[fd]\n",strlen("[name]\t[fd]\n"),0);
    for(int i=0;i<max_size;i++){
        if(socket_fds[i]!=0){
            char tmp[128];
            sprintf(tmp,"%s\t%d\n",names[socket_fds[i]],socket_fds[i]);
            send(fd,tmp,strlen(tmp),0);
        }
    }
    send(fd,"-------------------------\n",strlen("-------------------------\n"),0);
    fflush(stdout);
    printf("[%d] [%s] list all user\n",fd,names[fd]);
    fflush(stdout);
}

void send_to_all(char *buf){
    char *ptr=buf;
    while(*ptr!='\n'){
        ptr++;
    }
    *ptr='\0';
    for(int i=0;i<max_size;i++){
        if(socket_fds[i]!=0){
            send(socket_fds[i],buf,strlen(buf),0);
        }
    }
}



void print_manual(int fd){
    fflush(stdout);
    printf("[%d] [%s] LOGIN SUCCESS,send manual\n",fd,names[fd]);
    fflush(stdout);
    send(fd,"[Server] : LOGIN SUCCESS\n",strlen("[Server] : LOGIN SUCCESS\n"),0);
    send(fd,"----Manual page----\n",strlen("----Manual page----\n"),0);
    send(fd,"l    --->列出在線的人\n",strlen("l    --->列出在線的人\n"),0);
    send(fd,"@fd    --->選擇對戰的人\n",strlen("@fd    --->選擇對戰的人\n"),0);
    send(fd,"#(0-8)    --->下棋在哪個位子(只有棋局開始才可以使用)\n",strlen("#(0-8)    --->下棋在哪個位子(只有棋局開始才可以使用)\n"),0);
    send(fd,"bye    --->下線\n",strlen("bye    --->下線\n"),0);
    send(fd,"/all [msg] --->向所有人發話\n",strlen("/all [mag] --->向所有人發話\n"),0);
    send(fd,"/name [msg] --->私訊某人\n",strlen("/name [msg] --->私訊某人\n"),0);
    send(fd,"whoami --->list your name\n",strlen("whoami --->list your name\n"),0);
    send(fd,"man --->list manual page\n",strlen("man --->list manual page\n"),0);
    send(fd,"-------------------\n",strlen("-------------------\n"),0);
    char tmp[128];
    sprintf(tmp,"[Server] : %s已上線\n",names[fd]);
    fflush(stdout);
    printf("send [%d] [%s] online state to all user\n",fd,names[fd]);
    fflush(stdout);
    send_to_all(tmp);
}

void recv_invite(int fd,char *buf){
    int oppofd = atoi(buf+1);
    // printf("oppofd is %d",oppofd);
    if(play_state[fd]==1){
        char tmp[128];
        sprintf(tmp,"[Server] : you are playing, please send request after!");
        send(fd,tmp,strlen(tmp),0);
        return;
    }
    if(play_state[oppofd]==1){
        char tmp[128];
        sprintf(tmp,"[Server] : %s is playing, please send request after!",names[oppofd]);
        send(fd,tmp,strlen(tmp),0);
        return;
    }
    if(fd==oppofd){
        char tmp[128];
        sprintf(tmp,"[Server] : you can not invite yourself!!");
        send(fd,tmp,strlen(tmp),0);
        return;
    }
    int i;
    for(i=0;i<max_size;i++){
        if(oppofd==socket_fds[i]) break;
    }
    if(i==max_size){
        char tmp[128];
        sprintf(tmp,"[Server] : this user doesn't exist!!");
        send(fd,tmp,strlen(tmp),0);
        return;
    }
    char tmp[128];
    sprintf(tmp,"INVITE %d@%s@",fd,names[fd]);
    send(oppofd,tmp,strlen(tmp),0);
    fflush(stdout);
    printf("[%d] [%s] invites [%d] [%s]\n",fd,names[fd],oppofd,names[oppofd]);
    fflush(stdout);
}

void recv_yes(int fd,char *buf){
    int oppofd = atoi(buf+4);
    char tmp[128];
    if(play_state[oppofd]==1){
        sprintf(tmp,"playing : %s,game be cancel",names[oppofd]);
        send(fd,tmp,strlen(tmp),0);
        return;
    }
    play_state[fd]=1;
    play_state[oppofd]=1;
    // printf("oppofd is %d",oppofd);
    sprintf(tmp,"AGREE %d\b",fd);
    send(oppofd,tmp,strlen(tmp),0);
    fflush(stdout);
    printf("[%d] [%s] say yes to [%d] [%s]\n",fd,names[fd],oppofd,names[oppofd]);
    fflush(stdout);
}

void recv_no(int fd,char *buf){
    int oppofd = atoi(buf+3);
    send(oppofd,"REJECT ",7,0);
    fflush(stdout);
    printf("[%d] [%s] say no to [%d] [%s]\n",fd,names[fd],oppofd,names[oppofd]);
    fflush(stdout);
}

void recv_position(int fd,char *buf){
    int position = buf[1]-'0';
    int oppofd = atoi(buf+3);
    char tmp[128];
    sprintf(tmp,"#%d",position);
    send(oppofd,tmp,strlen(tmp),0); 
    fflush(stdout);
    printf("[%d] [%s] , [%d] [%s] are in the game\n",fd,names[fd],oppofd,names[oppofd]);
    printf("[%d] [%s] choose %d\n",fd,names[fd],position);
    fflush(stdout);
}

void recv_LOSE(int fd,char *buf){
    int cnt=0;
    char str_oppofd[32];
    char *ptr=buf+5;
    while(*ptr!='#'){
        str_oppofd[cnt++]=*ptr++;
    }
    str_oppofd[cnt]='\0';
    int oppofd = atoi(str_oppofd);
    char position = *(ptr+1);
    char tmp[32];
    sprintf(tmp,"LOSE %c\n",position);
    send(oppofd,tmp,strlen(tmp),0);
    play_state[fd]=0;
    play_state[oppofd]=0;
    fflush(stdout);
    printf("[%d] [%s] Lose [%d] [%s],game over\n",oppofd,names[oppofd],fd,names[fd]);
    fflush(stdout);
}

void recv_FAIR(int fd,char *buf){
    int oppofd=atoi(buf+5);
    send(oppofd,"FAIR",4,0);
    play_state[fd]=0;
    play_state[oppofd]=0;
    fflush(stdout);
    printf("[%d] [%s] and [%d] [%s] fair,game over\n",fd,names[fd],oppofd,names[oppofd]);
    fflush(stdout);
}


void recv_leave(int fd,char *buf){
    int oppofd = atoi(buf+6);
    if(play_state[fd]==1){
        play_state[fd]=0;
    }
    if(play_state[oppofd]==1){
        play_state[oppofd]=0;
    }
    char tmp[128];
    sprintf(tmp,"[Server] : [%s]已離線\n",names[fd]);
    send_to_all(tmp);
}



void send_private_mag(int fd,char *buf){
    char *ptr=buf+1;
    char tmp_name[128];
    int sendfd=-1;
    int cnt=0;
    while(*ptr!=' '){
        if(*ptr=='\n'){
            send(fd,"[Server] : Wrong msg format 名字跟訊息中間要有空格!\n",strlen("[Server] : Wrong msg format 名字跟訊息中間要有空格!\n"),0);
            send(fd,"--format is--\n",strlen("--format is--\n"),0);
            send(fd,"/all [msg]  --->send to all user\n",strlen("/all [msg]  --->send to all user\n"),0);
            send(fd,"/name [msg]  --->send private msg to someone\n",strlen("/name [msg]  --->send private msg to someone\n"),0);
            send(fd,"-------------",strlen("-------------"),0);
            return;
        }
        tmp_name[cnt++]=*ptr++;
    }
    tmp_name[cnt]='\0';
    printf("send name : %s\n",tmp_name);
    int i;
    for(i=0;i<max_size;i++){
        if(strcmp(names[i],tmp_name)==0){
            sendfd=i;
            break;
        }
    }
    if(i==max_size){
        send(fd,"[Server] : Sorry,We don't find this player online!",strlen("[Server] : Sorry,We don't find this player online!"),0);
        return;
    }
    if(sendfd==fd){
        send(fd,"[Server] : 阿你私訊自己要幹嘛@@",strlen("[Server] : 阿你私訊自己要幹嘛@@"),0);
        return;
    }
    char tmp[1024];
    char *qtr=ptr;
    while(*qtr!='\n'){
        qtr++;
    }
    *qtr='\0';
    sprintf(tmp,"private--->[%s] : %s",names[fd],ptr+1);
    send(sendfd,tmp,strlen(tmp),0);
    fflush(stdout);
    printf("[%d] [%s] send private msg to [%d] [%s] \n",fd,names[fd],sendfd,names[sendfd]);
    fflush(stdout);
}