//////////////////////////////////////////////////
// File name : server.c
// Date      :  2018.06.03
// Os        : Ubuntu 16.04 LTS 64 bits
// Author    : Park Siyeon
// ID        : 2016722094
//////////////////////////////

// Title : System Programmning Assignment # 3-2 
//   use thread to write logfile 



#include <netdb.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/sem.h>
#include <sys/ipc.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <pwd.h>
#include <dirent.h>
#include <time.h>
#include <openssl/sha.h>
#include <fcntl.h>
#include <pthread.h>

#define BUFFSIZE 512
#define PORTNO 38018

int semid;
time_t lgnow; 
int sub_count = 0; // to write log file 

int v();
int p();
char *getHomeDir(char * home); 
void WriteLog(char* url, char * hashed, int hit);
int  Proxy_Cache(char *input_url,char *hashed_url);
int Miss(char* url, char *request,char *hashed_url);
int Hit(int socket,char *hashed_url);
char *getIPAddr(char *addr);
static void handler()
{
	pid_t pid;
	int status;
	while( ( pid = waitpid(-1, &status, WNOHANG))>0);
}
void m_alarm(int signo) //alarm function
{
	printf("time out : Error\n");
	alarm(1);
	signal(SIGALRM, SIG_DFL);
}

void *t_thread_hit(void * url) //thread function hit
{
	char home[BUFFSIZE] = { 0. };
	getHomeDir(home);//get home dir
	strcat(home, "/logfile");
	mkdir(home, S_IRWXU | S_IRWXG | S_IRWXO);//make log dir 
	strcat(home, "/log");

	FILE * logfp = fopen(home, "a"); //open logfile 

	printf("*TID# %d.\n", (int)pthread_self());// to print consol
	time(&lgnow);//to write time 
	struct tm * ltp = localtime(&lgnow);
	
	fprintf(logfp, "[HIT]");
	fprintf(logfp, "%s",(char *) url);
	// year month day hour:min:sec
	fprintf(logfp, "-[%d/%d/%d %02d:%02d:%02d]\n"
		, ltp->tm_year + 1900, ltp->tm_mon + 1, ltp->tm_mday
		, ltp->tm_hour, ltp->tm_min, ltp->tm_sec);

	fclose(logfp);
}

void *t_thread_miss(void * url) //thread function miss
{
	char home[BUFFSIZE] = { 0. };
	getHomeDir(home);//get home dir
	strcat(home, "/logfile");
	mkdir(home, S_IRWXU | S_IRWXG | S_IRWXO);//make log dir 
	strcat(home, "/log");

	FILE * logfp = fopen(home, "a");

	printf("*TID# %d.\n", (int)pthread_self());// to print 
	time(&lgnow);
	struct tm	* ltp = localtime(&lgnow);
	fprintf(logfp, "[MISS] ");
	fprintf(logfp, "%s", (char *)url);
	fprintf(logfp, "-[%d/%d/%d %02d:%02d:%02d]\n"
		, ltp->tm_year + 1900, ltp->tm_mon + 1, ltp->tm_mday
		, ltp->tm_hour, ltp->tm_min, ltp->tm_sec);

	fclose(logfp);
}


void *t_thread_time(void * time)//thread function terminated 
{
	char home[BUFFSIZE] = { 0. };
	getHomeDir(home);//get home dir
	strcat(home, "/logfile");
	mkdir(home, S_IRWXU | S_IRWXG | S_IRWXO);//make log dir 
	strcat(home, "/log");

	FILE * logfp = fopen(home, "a");

	printf("*TID# %d.\n", (int)pthread_self());// to print 
	fprintf(logfp, "**SERVER** [Terminated] ");
	fprintf(logfp, "rum time: %d sec, #sub process: %d\n", *(int *)time, sub_count);

	fclose(logfp);
}


int main()
{
	struct sockaddr_in server_addr, client_addr;
	int socket_fd,client_fd;
	int len, len_out;
	int state;

	char buf[1024];
	char hashed[128]={0,};
	bzero(hashed,sizeof(hashed));
	
	pid_t pid;
	struct tm *ltp;
	time_t start,end;
	time(&start);

	//inti semaphore
	semid = semget((key_t)38018, 1, IPC_CREAT|IPC_EXCL|0666);
	int status = semctl(semid, 0, SETVAL, 1);

	if( (socket_fd = socket(PF_INET, SOCK_STREAM, 0 ))<0)
		{
			printf("Server : Can't open stream socket \n");
			return 0;
		}

	bzero( (char*)&server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr=htonl(INADDR_ANY);
	server_addr.sin_port=htons(PORTNO);

	if(bind (socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr))<0)
	{
		printf("Server : Can't bind local address \n");
		close(socket_fd);
		return 0;
	}

	listen(socket_fd, 5);
	signal(SIGCHLD,(void *) handler); //wait client socket


	state = 0;
	while(1)
	{

		struct in_addr inet_client_address;
		
		char response_header[1024]={0,};
		char response_message[1024]={0,};

		char tmp[1024]={0,};
		char method[20]={0,};
		char url[1024]={0,};

		char *tok=NULL;

		len=sizeof(client_addr);
		client_fd=accept(socket_fd,(struct sockaddr*)&client_addr,&len);
		if(client_fd<0)
		{
			printf("Server : accept failed\n");
			return 0;
		}

		if (state < 5)//to avoid infinite process
			pid = fork();// make child process
		else
		{
			sleep(5);
			state = 0;
		}
		if(pid == -1) //if error
		{
			close(client_fd);
			close(socket_fd);
			continue;
		}

		else if(pid == 0)//child process start
		{	
			read(client_fd,buf, 1024);
			
		
			strcpy(tmp,buf);
			tok = strtok(tmp," ");
			strcpy(method, tok);
			
			if (strcmp(method, "GET") == 0) //receive HTTP request 
			{
				tok = strtok(NULL, " ");
				strcpy(url, tok);
				// Get  url
			
				strcpy(tmp, buf);
				tok = strstr(tmp, "Host:");
				
				tok = strtok(tok, " ");
				tok = strtok(NULL, " ");
				tok = strtok(tok, "\r\n");
				// Get host
			}
			else
			{
				return -1;
			}
			// Get  urlmake 
			if(Proxy_Cache(url,hashed)==0) //if miss
			{	
				Miss(tok,buf,hashed);
			}	

			Hit(client_fd,hashed);

			return 0;		
			WriteLog("bye","bye",0);// to increase sub count
		}

		close(client_fd);
	}
	time(&end);
	close(socket_fd);
	WriteLog("bye","bye",end-start); // write time 
	return 0;

}



char *getHomeDir(char * home){ //to get home dir 
	struct passwd *usr_info=getpwuid(getuid());
	strcpy(home,usr_info->pw_dir);

	return home;
}

int  Proxy_Cache(char *input_url ,char * hashed_url){ // make url to hash and check hit or miss
	
	unsigned char hashed_160bits[20]={0,};
	char hashed_hex[41]={0,};
	int i;
	bzero(hashed_url, sizeof(hashed_url));

	SHA1(input_url, strlen(input_url), hashed_160bits);

	for (i = 0; i<sizeof(hashed_160bits); i++)
		sprintf(hashed_hex + i * 2, "%02x", hashed_160bits[i]);
	strcpy(hashed_url, hashed_hex);
	
	//hashed 

	FILE *fp=NULL;	
	char dirname[64] = { 0, };
	strncpy(dirname,hashed_url,3);
	char filename[64] = { 0, };
	strcpy(filename,hashed_url + 3); //separated by directory and file name


	int hit = 0;
	char home[128] = { 0, };

	bzero(home,sizeof(home));  
	getHomeDir(home);//get home dir
	strcat(home,"/cache/");
	
	strcpy(hashed_url, home);
	strcat(hashed_url, dirname);
	strcat(hashed_url, "/");
	strcat(hashed_url, filename);

	umask(0);// reset umask vaule
	mkdir(home,S_IRWXU | S_IRWXG | S_IRWXO);//make cache dir 
	strcat(home,dirname);// home will be "/home/~/cache/dir



	char slash[2]={"/"};// I need '/' string
	strcat(dirname, slash);
	strcat(dirname,filename);
	

	if(-1==mkdir(home,S_IRWXU | S_IRWXG | S_IRWXO)) 
		// if the directory already existed
	{
		struct dirent *pFile;
		DIR *pDir=opendir(home);
		// find if there are same file 
		for(pFile=readdir(pDir);pFile;pFile=readdir(pDir))
			if(strcmp(pFile->d_name,filename)==0)
			{//HIT
					closedir(pDir);
					WriteLog(input_url, hashed_url, -1);
					
					return -1;
			}
	}// MISS
	
		WriteLog(input_url, hashed_url, 0);
		return hit;

}
void WriteLog(char* url, char * hashed, int hit)
{

	int pid = getpid();
	int *time = &hit;
	int status;
	int err;
	void *tret;

	pthread_t tid;
	printf("*PID# %d is waiting for the semaphore\n", pid);

	p(); // p action
	sleep(2); // to  check semaphore
	printf("*PID# %d is in the critical zone \n",pid); // critical zone 
	
	printf("*PID# %d create the ",pid);

	if(strcmp(url,"bye") == 0) // if parent process 
		{
			if(hit ==0)
				{	sub_count++; return; }// just add sub process count
			err = pthread_create(&tid, NULL,t_thread_time,time);
			
			v();//  v actionss
			return;
		}
	if(hit == -1) // if hit
	{	
		err = pthread_create(&tid, NULL,t_thread_hit, url);
	}
	else if(hit== 0 )//if miss
	{	
		err = pthread_create(&tid, NULL,t_thread_miss, url);
	}
	
	pthread_join(tid,&tret);//ternimate thread 
	printf("*TID# %d is exited \n",(int)pthread_self());// to print 
	v();//v 

	return;
}	

char *getIPAddr(char *addr) // host to IP address
{
	struct hostent* hent;
	char * haddr;
	int len = strlen(addr);

	if( (hent = ( struct hostent*)gethostbyname(addr)) != NULL)
		haddr= inet_ntoa(*((struct in_addr*)hent->h_addr_list[0]));
	
	return haddr;

}
int Miss(char *url,char *request,char *hashed_url)//receive HTTP request and save at cache
{	
			
	int client_fd;
	struct sockaddr_in server_addr;
	char * haddr=getIPAddr(url);
	char buf[BUFFSIZE] = { 0, };


	if( (client_fd = socket(PF_INET, SOCK_STREAM, 0)) == -1 ){
		printf("Can't create socket.\n");
		return -1;
	}

	
	bzero( (char*)&server_addr, sizeof(server_addr) );

	server_addr.sin_family =AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(haddr);
	server_addr.sin_port = htons(80); 			// make socket information

	if( connect(client_fd, (struct sockaddr*)&server_addr, sizeof(server_addr))==-1){ //connect
		printf("can't connect \n");
		return -1;
	}
	

	

	if (write(client_fd,request, 1024)==-1)//send
			return -1;

	int fp = creat(hashed_url, 0666);
	signal(SIGALRM, m_alarm);//set signal 
	alarm(10);//if time overs 10 seconds this progarm exit
	while (read(client_fd, buf, 1) > 0)
	{
		write(fp, buf, 1); // write on  file 
	}
	alarm(0); //clear alarm
	close(client_fd);
	close(fp);

}
int Hit(int socket,char *hashed_url) //if HIT
{
	
	int fp = open(hashed_url, O_RDONLY);
	char temp[BUFFSIZE]={0,};
	bzero(temp,sizeof(temp));

	
	while (read(fp, temp, 1 )> 0) //read cache file 
	{
		write(socket, temp, 1);// and send web browser
	}
	

	close(fp);

	
	return 0;
}

int v() 
{
	struct sembuf vbuf; //
	vbuf.sem_num = 0;
	vbuf.sem_op = 1; // get out of critical section
	vbuf.sem_flg = SEM_UNDO;

	if (semop(semid, &vbuf, 1) < 0)
		return -1;
	else
		return 0;
}
int p() 
{
	struct sembuf pbuf;
	pbuf.sem_num = 0;
	pbuf.sem_op = -1; // enter critical section
	pbuf.sem_flg = SEM_UNDO;

	if (semop(semid, &pbuf, 1) < 0)
		return -1;
	else
		return 0;

}
