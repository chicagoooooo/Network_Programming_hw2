#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
int oppofd;
int request;
void handle_client(int fd,char *name_and_passwd);
void* recv_thread(void*);
void print_OX();
void init_OX_game();
void get_name_and_oppofd(char *name,char *buf);
int win();
int fair();
int get_oppofd(char *buf);
char OX[9];
int turn;
int Game;
int start;
char me,oppo;
char myname[32];
void print_manual();
int main(){
	int socketfd = socket(PF_INET,SOCK_STREAM,0);
    char buf[32];
	struct sockaddr_in cli_addr;
    char name_and_passwd[32]={0};
    char passwd[32];
	cli_addr.sin_family = PF_INET;
	cli_addr.sin_port = htons(10003);
	cli_addr.sin_addr.s_addr=htonl(INADDR_ANY);

	if (connect(socketfd,(struct sockaddr*)&cli_addr,sizeof(cli_addr)) == -1){
		perror("無法連接");
		exit(-1);
	}
	printf("--> Client start Successfully !\n");

	printf("--> 請輸入帳號 : ");
	scanf("%s",myname);
	printf("--> 請輸入密碼 : ");
	scanf("%s",passwd);
    strcat(name_and_passwd,myname);
	strcat(name_and_passwd,":");
	strcat(name_and_passwd,passwd);
    strcat(name_and_passwd,"$");
    handle_client(socketfd,name_and_passwd);
    
	return 0;
}


void handle_client(int fd,char *name_and_passwd){
    char buf[256];
	recv(fd,buf,sizeof(buf),0);
    while(1){
        // printf("client recv : %s\n",buf);
        if (strncmp(buf,"authenticate",strlen("authenticate") ) == 0){
		    send(fd,name_and_passwd,strlen(name_and_passwd),0);
            break;
        }
    }
    pthread_t id;
	pthread_create(&id,0,recv_thread,&fd);

    while(1){
        char buffer[1024];
        fgets(buffer,1024,stdin);
        // printf("buffer = %s",buffer);
        if(start==1){
            printf("---your turn begin---you are [%c]---\n",me);
            print_OX();
            printf("[Server] : please choose a position, format is #(0-8)\n");
            start=0;
        }

        if(buffer[0]=='l' && strlen(buffer)==2){
            send(fd,buffer,1024,0);
        }else if(buffer[0]=='@'){//invite
            send(fd,buffer,1024,0);
            if(Game==1){
                continue;
            }
            oppofd = get_oppofd(buffer);
            printf("[Server] : wait your opponent response...\n");
        }else if(strncmp(buffer,"bye",3)==0){//bye
            printf("[Server] : byeeeee\n");
            char tmp[128];
            sprintf(tmp,"LEAVE %d\n",oppofd);
            send(fd,tmp,strlen(tmp),0);
            exit(3);
        }else if(strncmp(buffer,"yes",3)==0 && request==1){
            request=0;
            printf("[Server] : Game Start!\n");
            char tmp[128];
            sprintf(tmp,"yes %d\n",oppofd);
            send(fd,tmp,strlen(tmp),0);
            Game=1;
            turn=1;
            start=1;
            me='O';
            oppo='X';
            init_OX_game();
            printf("[Server] : press enter to continue...\n");
        }else if(buffer[0]=='#'){
            if(Game==0) {
                printf("[Server] : Game is not starting...\n");
                continue;
            }
            if(turn==0) {
                printf("[Server] : its not your turn\n");
                continue;
            } 

            int position = atoi(buffer+1);
            if(OX[position]!=' ' || !(position <= 8 && position >= 0)){
                printf("[Server] : invalid posotion,please input again!!\n");
                continue;
            }
            OX[position] = me;
            if(fair()){
                printf("[Server] : its FAIR!!\n");
                Game=0;
                char tmp[128];
                sprintf(tmp,"FAIR %d\n",oppofd);
                send(fd,tmp,strlen(tmp),0);
                continue;
            }else{
                if(win()){
                    print_OX();
                    printf("[Server] : you win!!,YOU ARE WINNER\n");
                    Game=0;
                    char tmp[128];
                    sprintf(tmp,"LOSE %d#%d\n",oppofd,position);
                    send(fd,tmp,strlen(tmp),0);
                    continue;
                }
            }
            turn=0;
            print_OX();
            printf("----------your turn end----------\n");
            char tmp[128];
            sprintf(tmp,"#%d %d\n",position,oppofd);
            // printf("%s\n",tmp);
            send(fd,tmp,strlen(tmp),0);
        }else if(strcmp(buffer,"no\n")==0 && request==1){
            request=0;
            char tmp[128];
            sprintf(tmp,"no %d\n",oppofd);
            send(fd,tmp,strlen(tmp),0);
        }else if(strncmp(buffer,"/all ",5)==0){
            char tmp[4096];
            sprintf(tmp,"all-->[%s] : %s\n",myname,buffer+5);
            send(fd,tmp,strlen(tmp),0);
        }else if(buffer[0]=='/'){
            char tmp[1025];
            sprintf(tmp,"%s\n",buffer);
            send(fd,tmp,strlen(tmp),0);
        }else if(strcmp(buffer,"whoami\n")==0){
            printf("%s\n",myname);
        }else if(strcmp(buffer,"man\n")==0){
            print_manual();
        }else{
            if(buffer[0]!='\n'){
                printf("[Server] : 抱歉,您的輸入格式似乎有誤,以下是操作介面方法\n");
                print_manual();
            }
        }
    }
    close(fd);
	
}

void* recv_thread(void* a){
    int fd = *(int *)a; 
    while(1){
		char buf[1024] = {};
		if (recv(fd,buf,sizeof(buf),0) <= 0){
			return 0;
	    }
        // printf("buf = %s\n",buf);
        if(strncmp(buf,"INVITE ",7)==0){
            char name[128];
            get_name_and_oppofd(name,buf);
            printf("[Server] : %s is invite you to play!\n",name);
            printf("[Server] : input [yes],or [no]\n");
            request=1;
        }else if(strncmp(buf,"AGREE ",6)==0){
            oppofd = atoi(buf+6);
            printf("[Server] : Game Start!!\n");
            Game=1;
            turn=0;
            me='X';
            oppo='O';
            init_OX_game();
            if(turn) printf("[Server] : press enter to continue...\n");
            else printf("[Server] : wait your opponent...\n");
        }else if(buf[0]=='#'){
            if(Game==0) printf("[Server] : Game is not starting...\n");
            turn=1;
            int posotion = buf[1]-'0';
            OX[posotion] = oppo;
            printf("---your oppo turn---your oppo is---[%c]---\n",oppo);
            print_OX();
            fflush(stdout);
            printf("---your turn begin---you are [%c]---\n",me);
            printf("[Server] : please choose a position, format is #(0-8)\n");
            fflush(stdout);
        }else if(strncmp(buf,"LOSE",4)==0){
            int pos = buf[5]-'0';
            OX[pos] = oppo;
            fflush(stdout);
            print_OX();
            printf("[Server] : you lose!!,YOU ARE LOSER\n");
            fflush(stdout);
            Game=0;
        }else if(strncmp(buf,"LOGIN FAIL",10)==0){
            printf("[Server] : LOGIN FAIL!!TERMINATE CLIENT PROCESS\n");
            exit(3);
        }else if(strncmp(buf,"FAIR",4)==0){
            printf("[Server] : its FAIR\n");
        }else if(strncmp(buf,"REJECT ",7)==0){
            printf("[Server] : REJECT\n");
        }else if(strncmp(buf,"playing",7)==0){
            Game=0;
            start=0;
            printf("[Server] : %s\n",buf);
        }else{
            printf("%s\n",buf);
        }
    }
}

void print_OX(){
    printf("%c|%c|%c        0|1|2\n",OX[0],OX[1],OX[2]);
	printf("-----        -----\n");
	printf("%c|%c|%c        3|4|5\n",OX[3],OX[4],OX[5]);
	printf("-----        -----\n");
	printf("%c|%c|%c        6|7|8\n",OX[6],OX[7],OX[8]);
}

void init_OX_game(){
    for(int i=0;i<9;i++){
        OX[i]=' ';
    }
}


int win(){
    if(OX[0]==me && OX[1]==me && OX[2]==me) return 1;
    else if(OX[3]==me && OX[4]==me && OX[5]==me) return 1;
    else if(OX[6]==me && OX[7]==me && OX[8]==me) return 1;
    else if(OX[0]==me && OX[3]==me && OX[6]==me) return 1;
    else if(OX[1]==me && OX[4]==me && OX[7]==me) return 1;
    else if(OX[2]==me && OX[5]==me && OX[8]==me) return 1;
    else if(OX[0]==me && OX[4]==me && OX[8]==me) return 1;
    else if(OX[2]==me && OX[4]==me && OX[6]==me) return 1;
    else return 0;
}
int fair(){
    int i;
    for(i=0;i<9;i++){
        if(OX[i]==' ') break;
    }
    if(i==9 && (!win())) return 1;
    else return 0;
}


void get_name_and_oppofd(char *name,char *buf){
    char *ptr=buf+7;
    char str_oppofd[128];
    int cnt=0;
    while(*ptr!='@'){
        str_oppofd[cnt++]=*ptr++;
    }
    str_oppofd[cnt]='\0';
    // printf("%s\n",str_oppofd);
    oppofd=atoi(str_oppofd);
    ptr++;
    cnt=0;
    while(*ptr!='@'){
        name[cnt++]=*ptr++;
    }
    name[cnt]='\0';
    // printf("%s\n",name);
}

int get_oppofd(char *buf){
    char *ptr=buf+1;
    return atoi(ptr);
}

void print_manual(){
    printf("-------------------\n");
    printf("l    --->列出在線的人\n");
    printf("@fd    --->選擇對戰的人\n");
    printf("#(0-8)    --->下棋在哪個位子(只有棋局開始才可以使用)\n");
    printf("bye    --->下線\n");
    printf("/all [msg] --->向所有人發話\n");
    printf("/name [msg] --->私訊某人\n");
    printf("whoami --->list your name\n");
    printf("man --->list manual page\n");
    printf("-------------------\n");
}