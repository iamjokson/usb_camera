//#include <iostream>
#include <stdio.h>
#include <time.h>
#include <pthread.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include "usb_camera.h"
#include <string.h>
#include <dirent.h>
int port = 5110;
#define    MAXLINE        1024
#define FALSE  -1   
#define TRUE   0  
    
/* signal fresh frames */
pthread_mutex_t mutex_frame        	= PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  cond_frame 		= PTHREAD_COND_INITIALIZER;

/* global JPG frame */
unsigned char *pic_buf = NULL;
char *ip_recv = NULL;
int i;
unsigned char buffer[3000*1024];
int pic_size = 0;
int w = 0;
int h = 0;
int flag_time = 0;
int flag_set = 0;
int flag_pic  =0;
int flag_save = 0;
//int iAddrLen = sizeof(struct sockaddr);
//int iSocketClient;
//int iSocketClientNew;
struct sockaddr_in tSocketServerAddr;
struct sockaddr_in tSocketClientAddr;
void Id(char *rcv)
{
        int fd;
        int err;
        char rcv_buf[100];
//      char *rcv;
        char * imei="AT+CGSN\r";
        fd = UART0_Open(fd,"/dev/ttyUSB0"); //′ò?a′??ú￡?·μ?????t?èê?·?  
      do{
                  err = UART0_Init(fd,115200,0,8,1,'N');
                  printf("Set Port Exactly!\n");
       }while(FALSE == err || FALSE == fd);
        UART0_Send_IMEI(fd,imei,rcv_buf);
        printf("%s\n",rcv_buf);
        char *p;
        p = strtok(rcv_buf," ");
        if(p)
        printf("%s\n",p);
        p = strtok(NULL," ");
        printf("%s\n",p);
        strncpy(rcv,p,15);
        printf("rec=%s\n",rcv);
        printf("strlen = %d\n",strlen(rcv));
        UART0_Close(fd);
}

void *ser_rev_thread(void *arg)
{
	printf("ser_rev_thread is start\n");
	int fd = (int)arg;
	int iAddrLen;
	int iRecvLen;
	unsigned char ucRecvBuf[40];
	while(1)
	{
		pthread_mutex_lock(&mutex_frame);
		iAddrLen = sizeof(struct sockaddr);
		iRecvLen = recvfrom(fd, ucRecvBuf, 999, 0, (struct sockaddr *)&tSocketServerAddr, &iAddrLen);
		if (iRecvLen > 0)
		{
			ucRecvBuf[iRecvLen] = '\0';
			printf("Get Msg From %s\n", ucRecvBuf);
		}
	  	char *port_recv;
	    ip_recv = strtok(ucRecvBuf,",");
	    if(ip_recv)
	    printf("%s\n",ip_recv);
	    port_recv=strtok(NULL,",");
	    if(port_recv)
	    printf("%s\n",port_recv);
	    i = atoi(port_recv);
	    printf("%d\n",i);
//		pthread_mutex_lock(&mutex_frame);
		pthread_cond_broadcast(&cond_frame);
		printf("broad to ........\n");
		pthread_mutex_unlock(&mutex_frame);
		usleep(1000);
	}

}
void *server_thread(void *arg)
{
//	pthread_detach(pthread_self());
	printf("server_thread start ------\n");
	int iSocketClient;
	struct hostent *h;
	char *ip;
	if((h=gethostbyname("qy2587.xicp.net"))==NULL)
	{
		fprintf(stderr,"不能得到IP/n");
		exit(1);
	}
	ip = inet_ntoa(*((struct in_addr *)h->h_addr));
	iSocketClient = socket(AF_INET, SOCK_DGRAM, 0);
	if (-1 == iSocketClient)
	{
		printf("socket error!\n");
//		goto QUIT;
	}
	/* char* IP;
        struct hostent *host = gethostbyname("qy2587.xicp.net");
        if(!host)
	 {
       	puts("Get IP address error!");
        	system("pause");
        	return -1;
        }*/
//	printf("IP addr %s\n", (*(struct in_addr*)host->h_addr_list[0] ) );
      // IP = inet_ntoa( *(struct in_addr*)host->h_addr_list[0] );
	tSocketServerAddr.sin_family      = AF_INET; 
	tSocketServerAddr.sin_port        = htons(port); /* host to net, short */
 //	tSocketServerAddr.sin_addr.s_addr = INADDR_ANY;
	if (0 == inet_aton(ip, &tSocketServerAddr.sin_addr))
 	{
		printf("invalid server_ip\n");
	}
	memset(tSocketServerAddr.sin_zero, 0, 8);
    char buf[15];
    Id(buf);
    buf[15] = '\0';
    printf("buf = %s\n",buf);
    printf("strlen = %d\n",strlen(buf));

	char buf_send[30];
	sprintf(buf_send,"SB,%15s,1",buf);
	printf("buf_send =%s\n",buf_send);
	buf_send[20] = '\0';
//	char buf_send[20] = "SB,111,1";
	printf("buf_send =%s\n",buf_send);
//	unsigned char ucRecvBuf[1000];
	int ret;
	int iRet;
	pthread_t pid;
	int iAddrLen;
	int iRecvLen;
	iAddrLen = sizeof(struct sockaddr);

	iRet = pthread_create(&pid, NULL, ser_rev_thread, (void *)iSocketClient);
	if(iRet != 0)
	{
		printf("create pthread failed\n");	
	}
	else
	{
		printf("server pthread ser_rev_thread success\n");
	}
//	usleep(500);
	while(1)
	{
		printf("to server\n");
		ret = sendto(iSocketClient, buf_send, strlen(buf_send), 0,(const struct sockaddr *)&tSocketServerAddr, iAddrLen);	
		if (ret < 0)
		{
			fprintf(stderr,"send to server error\n");
		}
		else
		{
			fprintf(stderr,"send for server success\n");
		}
		sleep(60);
	}
		printf("send for server after\n");
}
void *Save_Send_thread(void *arg)
{
	int fd = (int )arg;
	static int num = 0;
	int f=0;
	unsigned int camSize;
	while(1){
		usleep(1000);
		if(flag_set == 1)
		{		
			continue;
		}
		if(flag_pic == 1)
		{
			flag_pic = 0;
			sleep(1);
			camSize = getframe();
			printf("camSize = %d\n",camSize);
			send_picture(fd,camSize);	
		}
		if((flag_save == 1)&&(flag_time == 1))
		{
			flag_time = 0;
			printf("flag_time == %d\n",flag_time);
			printf("start save picture\n");
		//	camSize = getframe();
			SaveSd();
			
		}
	}
}


void *client_thread(void *arg)
{
		printf("client_thread start -----\n");
		char buf[20]= "hello";
		int iRet;
		int s;
		int flag = 0;
		int maxfdp = 0;
		pthread_t pid;
		unsigned int camSize;
		unsigned int nbytes;
		int iSocketClientNew;
		int val;
		int tmp  = 3;
		int iAddrLen;
		char location[100];
		int iRecvLen;
		fd_set fds;
		int ret;
		FILE* fp;
		iAddrLen = sizeof(struct sockaddr);
		while(1)
		{
			pthread_mutex_lock(&mutex_frame);
			pthread_cond_wait(&cond_frame, &mutex_frame);
			printf("wait signal success\n");
			
			iSocketClientNew = socket(AF_INET, SOCK_STREAM, 0);
			printf("iSocketClientNew = %d\n",iSocketClientNew);
			if (-1 == iSocketClientNew)
			{
				printf("socket error!\n");
			//		goto QUIT;
			}
			tSocketClientAddr.sin_family      = AF_INET; 
			tSocketClientAddr.sin_port        = htons(i); /* host to net, short */
			
			if (0 == inet_aton(ip_recv, &tSocketClientAddr.sin_addr))
			{
				printf("invalid server_ip\n");
			}
			memset(tSocketClientAddr.sin_zero, 0, 8);
			printf("i = %d,ip = %s\n",i,ip_recv);
			pthread_mutex_unlock(&mutex_frame);
			
			iRet = connect(iSocketClientNew, (const struct sockaddr *)&tSocketClientAddr, sizeof(struct sockaddr));	

			if (0 == iRet)
			{			
				printf("connect success\n");
			}	
			else
			{
				printf("connect error!\n");
			}
			
		ret = pthread_create(&pid, NULL, Save_Send_thread, (void *)iSocketClientNew);
		if(ret != 0)
		{
			printf("create pthread failed\n");	
		}
		else
		{
			printf("server pthread Save_Send_thread success\n");
		}	
		while(1)
		{
		int w;
        int h;
		unsigned char ucRecvBuf[40];
		iAddrLen = sizeof(struct sockaddr);

while(1)
{
		struct timeval timeout;
		timeout.tv_sec = tmp;
		timeout.tv_usec = 0;
        FD_ZERO(&fds); //每次循环都要清空集合，否则不能检测描述符变化  
        FD_SET(iSocketClientNew,&fds); //添加描述符   
        maxfdp=iSocketClientNew+1; //描述符最大值加1  
        printf("timeout.tv_sec = %d\n",timeout.tv_sec);
        switch(select(maxfdp,&fds,NULL,NULL,&timeout)) //select使用  
        {
            case -1: printf("select error");return ;break; //select错误，退出程序  
            case 0: printf("time out\n"); flag_time =1;break;
            default:
            if(FD_ISSET(iSocketClientNew,&fds)) //测试sock是否可读，即是否网络上有数据  
            {
              	iRecvLen = recv(iSocketClientNew, ucRecvBuf, 999, 0);
				if (iRecvLen > 0)
				{
					ucRecvBuf[iRecvLen] = '\0';
					printf("Get Msg From %s\n", ucRecvBuf);
				}
				
              if(strcmp("ni",ucRecvBuf)==0)
				{
					if((w != 640)&&(h != 480))
					{
						flag_set = 1;
						w = 640;
						h = 480;
						stop_cap_mjpg();
						uninit_camera();
						if(init_camera(w,h, "/dev/video0") < 0)
						{      
							printf("init camera failed\n");     
							return ;    
						} 
						flag_pic = 1;
						flag_set = 0;
						start_cap_mjpg(buffer);
				 		flag_pic = 0;
					}
					else
					{
						printf("w ============%d\n",w);
						printf("h ============%d\n",h);
						flag_pic = 1;
					}
				}
				else if(strcmp("hao",ucRecvBuf)==0)
				{
					printf("hao ------------\n");
						if((w != 1280)&&(h != 720))
					{
						flag_set = 1;
						w = 1280;
						h = 720;
						stop_cap_mjpg();
						uninit_camera();
						if(init_camera(w,h, "/dev/video0") < 0)
						{      
							printf("init camera failed\n");     
							return ;    
						} 
						flag_pic = 1;
						flag_set = 0;
						start_cap_mjpg(buffer);
				 		flag_pic = 0;
					}
					else
					{
						printf("w ============%d\n",w);
						printf("h ============%d\n",h);
						flag_pic = 1;
					}
				}
				else if(strcmp("kan",ucRecvBuf)==0)
				{
					printf("start kan--------------------\n");

					See(iSocketClientNew);
				}
				else if(strncmp("rm",ucRecvBuf,2)==0)
				{
					printf("start delete-----------------\n");
					RmFile(ucRecvBuf);
				}
				else if(strncmp("grep",ucRecvBuf,4)==0)
				{
					printf("start grep ----------------");
					Grep(ucRecvBuf,iSocketClientNew);
				}
				else if(strcmp("stop",ucRecvBuf)==0)
				{
					flag_save = 0;
					printf("stop save\n");
				}
				else if(strncmp("save",ucRecvBuf,4)==0)
				{
				char * aa;
				char * file;
				aa= strtok(ucRecvBuf,"+");
				if(aa)
				printf("%s\n",aa);
				file=strtok(NULL,"\0");
				if(file)
				{
					printf("file = %s\n",file);
				}
					tmp= atoi(file);
					printf("tmp = %d\n",tmp);
					timeout.tv_sec = tmp;
					printf("tmp =========timeout.tv_sec = %d\n",timeout.tv_sec);
				//	flag_save = 1;
				
					if((w != 640)&&(h != 480)||(w != 1280)&&(h != 720))
					{
						flag_set = 1;
						w = 640;
						h = 480;
						stop_cap_mjpg();
						uninit_camera();
						if(init_camera(w,h, "/dev/video0") < 0)
						{      
							printf("init camera failed\n");     
							return ;    
						} 
						flag_set = 0;
						flag_save = 1;
						start_cap_mjpg(buffer);
						w = 0;
						h = 0;
					}	
					else
					{
						flag_save = 1;
						printf("save picture loading\n");
					}
				}  
			        }// end if break;  
			    }// end switch  
    }//end while  		
			}
		}
}

void send_picture(int fd,int size)
{
		//unsigned char buffer[300*1024];
		unsigned char *p = buffer;
		int ret;
		int times = 0;
		int cnt = 0;
	/*	ret = send(fd, &size, sizeof(size), 0);
		if(ret < 0)
		{
			fprintf(stderr,"send error\n");
		}
		printf("send fun brfore\n");
		printf("size = %d\n",size);
		*/
		while (size > 0)
		{	
			printf("in while\n");
			ret = send(fd, p, size, 0);
			if (ret == 0)
			{
				fprintf(stderr,"the client is disconnetcted\n");
				return ;
			}
			else if (ret < 0)
			{
					fprintf(stderr,"send error\n");
					
			}
			size -= ret;
			p += ret;
		}
	return ;
}
void GetTime(char *buffer)
{
     struct tm *t;
     time_t tt;
     time(&tt);
     t = localtime(&tt);
     sprintf(buffer,"/mnt/sd/%4d.%02d.%02d:%02d.%02d.%02d.jpg",t->tm_year + 1900, t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec);
//      printf("buffer = %s",buffer);
}
void SaveSd()
{
//	unsigned char buffer[300*1024];
	char location[30];
	int camSize;
	int nbytes;
	FILE * fp;
/*		if(init_camera(640,480, "/dev/video0") < 0){
		printf("init camera failed\n");
		}
		start_cap_mjpg(buffer);*/

		camSize = getframe();
		GetTime(location);
		printf("%s\n",location);
		printf("getframe %u\n",camSize);
		fp = fopen(location,"w");
		if(fp == NULL)
		{
			printf("open %s failed,%s\n",location,strerror(errno));
		}
		if((nbytes = fwrite(buffer,1,camSize,fp))!=camSize)
		{
			fclose(fp);
			printf("write %s %u bytes,error:%s\n", location, nbytes, strerror(errno));
	//		continue;
		}
		fclose(fp);
	//	stop_cap_mjpg();
    //	uninit_camera();
	
}
void See(int fd)
{
	flag_save = 0;
	printf("in kan func--------------\n");
	unsigned char name[3000*1024];
	unsigned char *p = name;
	int size = sizeof(name);
	DIR * dp; 
	int ret;
	struct dirent *filename;
    dp = opendir("/mnt/sd");
	if (!dp)
	{
       fprintf(stderr,"open directory error\n");
       return ;
	}
	while (filename=readdir(dp))
	{
	if((strcmp(filename->d_name,".") != 0)&&(strcmp(filename->d_name,"..") != 0))
		{
       	 sprintf(name,"%10s\n",
         filename->d_name);
		}
		printf("name = %s\n",name);
		printf("in while\n");
		ret = send(fd, &name, strlen(name), 0);
		if(ret < 0)
		{
			fprintf(stderr,"send error\n");
		}

		//flag_save = 1;
		}
		closedir(dp);
		printf("extt see---------------------\n");
}
void RmFile(char *filename)
{
	char * tmp;
	char * file;
	char loation[30];
	tmp= strtok(filename,"+");
    if(tmp)
    printf("%s\n",tmp);
    file=strtok(NULL,"\0");
    if(file)
    printf("%s\n",file);
	printf("The file to delete:");
	sprintf(loation,"/mnt/sd/%s",file);
    if( remove(loation) == 0 )
        printf("Removed %s.", file);
    else
        perror("remove");
}
void Grep(char *arg,int fd)
{
	char * tmp;
	unsigned char buf[300*1024];
	unsigned char *p = buf;
	char * file;
	char loation[50];
	FILE * fp;
	DIR * dp; 
	int ret;
	int size;
	int read_len;
	struct dirent *filename;
	tmp= strtok(arg,"+");
    if(tmp)
    printf("%s\n",tmp);
    file=strtok(NULL,"\0");
    if(file)
    printf("%s\n",file);
	dp = opendir("/mnt/sd/");
	if (!dp)
	{
       fprintf(stderr,"open directory error\n");
       return ;
	}
	while(filename=readdir(dp))
	{
	if((strcmp(filename->d_name,file) == 0))
		{
       	 sprintf(loation,"./mnt/sd/%10s",
         filename->d_name);
		 break;
		}
	}
	printf("loation = %s\n",loation);
    if ((fp = fopen(loation,"r+")) == NULL) 
	{
        perror("Open file failed\n");
        exit(0);
    }
	fseek(fp,0,SEEK_END);
	size = ftell(fp);
	printf("size == %d\n",size);
	fclose(fp);
	if ((fp = fopen(loation,"r+")) == NULL) 
	{
        perror("Open file failed\n");
        exit(0);
    }
		read_len = fread(buf, 1, size, fp);
		printf("read_len = %d\n",read_len);
	/*	ret = send(fd, &size, sizeof(size), 0);
		if(ret < 0)
		{
			fprintf(stderr,"send error\n");
		}
		printf("send fun brfore\n");
		printf("size = %d\n",size);
		*/
		while (size > 0)
		{	
			printf("in while\n");
			ret = send(fd, p, size, 0);
			if (ret == 0)
			{
				fprintf(stderr,"the client is disconnetcted\n");
				return ;
			}
			else if (ret < 0)
			{
					fprintf(stderr,"send error\n");
					
			}
			size -= ret;
			p += ret;
		}
	closedir(dp);

}
int main(void)
{
	unsigned int num = 0;
	int j;
	int w;
	int h;
	int iRecvLen;
	void *rval;
	pthread_t pid;
	pthread_t pid1;
	int iRet;
	int ret;
	int read_len;
//	unsigned char *p = buffer;
	connect_4g();
	iRet = pthread_create(&pid, NULL, server_thread, NULL);
	if(iRet != 0)
	{
		printf("create pthread failed\n");	
	}
	else
	{
		printf("server pthread server_thread success\n");
	}
//	sleep(2);
	while(1)
	{
		ret = pthread_create(&pid1, NULL, client_thread, NULL);
		if(ret != 0)
		{
			printf("create pthread failed\n");	
		}
		else
		{
			printf("server pthread client_thread success\n");
		}
		pthread_join(pid1,&rval);
		printf("client thread exit code	is %d\n",(int )rval);
	}
	getchar();
	sleep(2);
	stop_cap_mjpg();
    uninit_camera();
    return 0;
}
