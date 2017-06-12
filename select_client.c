#include <sys/select.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
int main()  
{  
    fd_set fds;  
    struct timeval timeout={3,0}; //select等待3秒，3秒轮询，要非阻塞就置0  
    char buffer[256]={0}; //256字节的接收缓冲区  
    /* 假定已经建立UDP连接，具体过程不写，简单，当然TCP也同理，主机ip和port都已经给定，要写的文件已经打开 
    sock=socket(...); 
    bind(...); 
    fp=fopen(...); */  
	int iSocketClient;
        struct sockaddr_in tSocketServerAddr;
        int iAddrLen;
        int iRet;
        unsigned char ucRecvBuf[1000];
        unsigned char ucSendBuf[10] = "hello";
        int iSendLen;
        int iRecvLen;
	 int maxfdp = 0;
        iSocketClient = socket(AF_INET, SOCK_STREAM, 0);
        tSocketServerAddr.sin_family      = AF_INET;
        tSocketServerAddr.sin_port        = htons(6007);  /* host to net, short */
        //tSocketServerAddr.sin_addr.s_addr = INADDR_ANY;
        if (0 == inet_aton("192.168.10.150", &tSocketServerAddr.sin_addr))
        {
                printf("invalid server_ip\n");

        }
        memset(tSocketServerAddr.sin_zero, 0, 8);
        iRet = connect(iSocketClient, (const struct sockaddr *)&tSocketServerAddr, sizeof(struct sockaddr));
        if (-1 == iRet)
        {
                printf("connect error!\n");

        }  
  while(1)  
    {  
        FD_ZERO(&fds); //每次循环都要清空集合，否则不能检测描述符变化  
        FD_SET(iSocketClient,&fds); //添加描述符   
        maxfdp=iSocketClient+1; //描述符最大值加1  
        switch(select(maxfdp,&fds,NULL,NULL,&timeout)) //select使用  
        {  
            case -1: printf("select error");return -1;break; //select错误，退出程序  
            case 0: printf("time out\n"); break;
            default:  
            if(FD_ISSET(iSocketClient,&fds)) //测试sock是否可读，即是否网络上有数据  
            {  
               // recvfrom(sock,buffer,256,.....);//接受网络数据      
                 iRecvLen = recv(iSocketClient, ucRecvBuf, 999, 0);
                                if (iRecvLen > 0)
                                {
                                        ucRecvBuf[iRecvLen] = '\0';
                                        printf("Get Msg From %s\n", ucRecvBuf);
                                }
		//buffer清空;  
            }// end if break;  
        }// end switch  
    }//end while  
}//end main  
