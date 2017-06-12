#include<stdio.h>      /*��׼�����������*/  
#include<stdlib.h>     /*��׼�����ⶨ��*/  
#include<unistd.h>     /*Unix ��׼��������*/  
#include<sys/types.h>   
#include<sys/stat.h>     
#include<fcntl.h>      /*�ļ����ƶ���*/  
#include<termios.h>    /*PPSIX �ն˿��ƶ���*/  
#include<errno.h>      /*����Ŷ���*/  
#include<string.h>  
   
   
//�궨��  
#define FALSE  -1  
#define TRUE   0  
   
/******************************************************************* 
* ���ƣ�                  UART0_Open 
* ���ܣ�                �򿪴��ڲ����ش����豸�ļ����� 
* ��ڲ�����        fd    :�ļ�������     port :���ں�(ttyS0,ttyS1,ttyS2) 
* ���ڲ�����        ��ȷ����Ϊ1�����󷵻�Ϊ0 
******************************************************************/  
int UART0_Open(int fd,char* port)  
{  
     
         fd = open( port, O_RDWR|O_NOCTTY|O_NDELAY);  
         if (FALSE == fd)  
                {  
                       perror("Can't Open Serial Port");  
                       return(FALSE);  
                }  
     //�ָ�����Ϊ����״̬                                 
     if(fcntl(fd, F_SETFL, 0) < 0)  
                {  
                       printf("fcntl failed!\n");  
                     return(FALSE);  
                }       
         else  
                {  
                  printf("fcntl=%d\n",fcntl(fd, F_SETFL,0));  
                }  
      //�����Ƿ�Ϊ�ն��豸      
      if(0 == isatty(STDIN_FILENO))  
                {  
                       printf("standard input is not a terminal device\n");  
                  return(FALSE);  
                }  
  else  
                {  
                     printf("isatty success!\n");  
                }                
  printf("fd->open=%d\n",fd);  
  return fd;  
}  
/******************************************************************* 
* ���ƣ�                UART0_Close 
* ���ܣ�                �رմ��ڲ����ش����豸�ļ����� 
* ��ڲ�����        fd    :�ļ�������     port :���ں�(ttyS0,ttyS1,ttyS2) 
* ���ڲ�����        void 
*******************************************************************/  
   
void UART0_Close(int fd)  
{  
    close(fd);  
}  
   
/******************************************************************* 
* ���ƣ�                UART0_Set 
* ���ܣ�                ���ô�������λ��ֹͣλ��Ч��λ 
* ��ڲ�����        fd        �����ļ������� 
*                              speed     �����ٶ� 
*                              flow_ctrl   ���������� 
*                           databits   ����λ   ȡֵΪ 7 ����8 
*                           stopbits   ֹͣλ   ȡֵΪ 1 ����2 
*                           parity     Ч������ ȡֵΪN,E,O,,S 
*���ڲ�����          ��ȷ����Ϊ1�����󷵻�Ϊ0 
*******************************************************************/ 

int UART0_Set(int fd,int speed,int flow_ctrl,int databits,int stopbits,int parity)  
{  
     
      int   i;  
      int   status;  
      int   speed_arr[] = { B115200, B19200, B9600, B4800, B2400, B1200, B300};  
      int   name_arr[] = {115200,  19200,  9600,  4800,  2400,  1200,  300};  
           
    struct termios options;  
     
    /*tcgetattr(fd,&options)�õ���fdָ��������ز������������Ǳ�����options,�ú��������Բ��������Ƿ���ȷ���ô����Ƿ���õȡ������óɹ�����������ֵΪ0��������ʧ�ܣ���������ֵΪ1. 
    */  
    if  ( tcgetattr( fd,&options)  !=  0)  
       {  
          perror("SetupSerial 1");      
          return(FALSE);   
       }  
    
    //���ô������벨���ʺ����������  
    for ( i= 0;  i < sizeof(speed_arr) / sizeof(int);  i++)  
                {  
                     if  (speed == name_arr[i])  
                            {               
                                 cfsetispeed(&options, speed_arr[i]);   
                                 cfsetospeed(&options, speed_arr[i]);    
                            }  
              }       
     
    //�޸Ŀ���ģʽ����֤���򲻻�ռ�ô���  
    options.c_cflag |= CLOCAL;  
    //�޸Ŀ���ģʽ��ʹ���ܹ��Ӵ����ж�ȡ��������  
    options.c_cflag |= CREAD;  
    
    //��������������  
    switch(flow_ctrl)  
    {  
        
       case 0 ://��ʹ��������  
              options.c_cflag &= ~CRTSCTS;  
              break;     
        
       case 1 ://ʹ��Ӳ��������  
              options.c_cflag |= CRTSCTS;  
              break;  
       case 2 ://ʹ�����������  
              options.c_cflag |= IXON | IXOFF | IXANY;  
              break;  
    }  
    //��������λ  
    //����������־λ  
    options.c_cflag &= ~CSIZE;  
    switch (databits)  
    {    
       case 5    :  
                     options.c_cflag |= CS5;  
                     break;  
       case 6    :  
                     options.c_cflag |= CS6;  
                     break;  
       case 7    :      
                 options.c_cflag |= CS7;  
                 break;  
       case 8:      
                 options.c_cflag |= CS8;  
                 break;    
       default:     
                 fprintf(stderr,"Unsupported data size\n");  
                 return (FALSE);   
    }  
    //����У��λ  
    switch (parity)  
    {    
       case 'n':  
       case 'N': //����żУ��λ��  
                 options.c_cflag &= ~PARENB;   
                 options.c_iflag &= ~INPCK;      
                 break;   
       case 'o':    
       case 'O'://����Ϊ��У��      
                 options.c_cflag |= (PARODD | PARENB);   
                 options.c_iflag |= INPCK;               
                 break;   
       case 'e':   
       case 'E'://����ΪżУ��    
                 options.c_cflag |= PARENB;         
                 options.c_cflag &= ~PARODD;         
                 options.c_iflag |= INPCK;        
                 break;  
       case 's':  
       case 'S': //����Ϊ�ո�   
                 options.c_cflag &= ~PARENB;  
                 options.c_cflag &= ~CSTOPB;  
                 break;   
        default:    
                 fprintf(stderr,"Unsupported parity\n");      
                 return (FALSE);   
    }   
    // ����ֹͣλ   
    switch (stopbits)  
    {    
       case 1:     
                 options.c_cflag &= ~CSTOPB; break;   
       case 2:     
                 options.c_cflag |= CSTOPB; break;  
       default:     
                       fprintf(stderr,"Unsupported stop bits\n");   
                       return (FALSE);  
    }  
     
  //�޸����ģʽ��ԭʼ�������  
  options.c_oflag &= ~OPOST;  
    
  options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);//�Ҽӵ�  
//options.c_lflag &= ~(ISIG | ICANON);  
     
    //���õȴ�ʱ�����С�����ַ�  
    options.c_cc[VTIME] = 1; /* ��ȡһ���ַ��ȴ�1*(1/10)s */    
    options.c_cc[VMIN] = 1; /* ��ȡ�ַ������ٸ���Ϊ1 */  
     
    //�����������������������ݣ����ǲ��ٶ�ȡ ˢ���յ������ݵ��ǲ���  
    tcflush(fd,TCIFLUSH);  
     
    //�������� (���޸ĺ��termios�������õ������У�  
    if (tcsetattr(fd,TCSANOW,&options) != 0)    
           {  
               perror("com set error!\n");    
              return (FALSE);   
           }  
    return (TRUE);   
}  
/******************************************************************* 
* ���ƣ�                UART0_Init() 
* ���ܣ�                ���ڳ�ʼ�� 
* ��ڲ�����        fd       :  �ļ�������    
*               speed  :  �����ٶ� 
*                              flow_ctrl  ���������� 
*               databits   ����λ   ȡֵΪ 7 ����8 
*                           stopbits   ֹͣλ   ȡֵΪ 1 ����2 
*                           parity     Ч������ ȡֵΪN,E,O,,S 
*                       
* ���ڲ�����        ��ȷ����Ϊ1�����󷵻�Ϊ0 
*******************************************************************/  
int UART0_Init(int fd, int speed,int flow_ctrl,int databits,int stopbits,int parity)  
{  
    int err;  
    //���ô�������֡��ʽ  
    if (UART0_Set(fd,115200,0,8,1,'N') == FALSE)  
       {                                                           
        return FALSE;  
       }  
    else  
       {  
               return  TRUE;  
        }  
}  
   
/******************************************************************* 
* ���ƣ�                  UART0_Recv 
* ���ܣ�                ���մ������� 
* ��ڲ�����        fd                  :�ļ�������     
*                              rcv_buf     :���մ��������ݴ���rcv_buf�������� 
*                              data_len    :һ֡���ݵĳ��� 
* ���ڲ�����        ��ȷ����Ϊ1�����󷵻�Ϊ0 
*******************************************************************/  
int UART0_Recv(int fd, char *rcv_buf,int data_len)  
{  
    int len,fs_sel;  
    fd_set fs_read;  
     
    struct timeval time;  
     
    FD_ZERO(&fs_read);  
    FD_SET(fd,&fs_read);  
     
    time.tv_sec = 10;  
    time.tv_usec = 0;  
     
    //ʹ��selectʵ�ִ��ڵĶ�·ͨ��  
    fs_sel = select(fd+1,&fs_read,NULL,NULL,&time);  
    if(fs_sel)  
       {  
              len = read(fd,rcv_buf,data_len);  
          printf("I am right!(version1.2) len = %d fs_sel = %d\n",len,fs_sel);  
              return len;  
       }  
    else  
       {  
          printf("Sorry,I am wrong!");  
              return FALSE;  
       }       
}  
/******************************************************************** 
* ���ƣ�                  UART0_Send 
* ���ܣ�                �������� 
* ��ڲ�����        fd                  :�ļ�������     
*                              send_buf    :��Ŵ��ڷ������� 
*                              data_len    :һ֡���ݵĸ��� 
* ���ڲ�����        ��ȷ����Ϊ1�����󷵻�Ϊ0 
*******************************************************************/  
int UART0_Send(int fd, char *send_buf,int data_len)  
{  
    int len = 0;  
     
    len = write(fd,send_buf,data_len);  
    if (len == data_len )  
              {  
                     return len;  
              }       
    else     
        {  
                 
                tcflush(fd,TCOFLUSH);  
                return FALSE;  
        }  
     
}  
void UART0_Send_Recv(int fd,char * AT_command)

{
	int len;	
	char rcv_buf[100];	
	len = UART0_Send(fd,AT_command,strlen(AT_command));  
	if(len > 0)  
	       printf("send data %s successful\n",AT_command);  
	else  
	       printf("send data failed!\n");                                 
	 len = UART0_Recv(fd, rcv_buf,30);  
	 if(len > 0)  
	{  
		rcv_buf[len] = '\0';  
		           printf("receive data is %s\n",rcv_buf);  
		printf("len = %d\n",len);  
	}  
	else  
	{  
	       printf("cannot receive data\n");  
	} 
	sleep(1);


}
void UART0_Send_IMEI(int fd,char * AT_command,char *rcv_buf)
{
	int len;	
//	char rcv_buf[100];	
	len = UART0_Send(fd,AT_command,strlen(AT_command));  
	if(len > 0)  
	       printf("send data %s successful\n",AT_command);  
	else  
	       printf("send data failed!\n");                                 
	 len = UART0_Recv(fd, rcv_buf,30);  
	 if(len > 0)  
	{  
		rcv_buf[len] = '\0';  
		           printf("receive data is %s\n",rcv_buf);  
		printf("len = %d\n",len);  
	}  
	else  
	{  
	       printf("cannot receive data\n");  
	} 
	sleep(1);

}
	
void connect_4g(void)  
{  
//    int err;                           //���ص��ú�����״̬  
    int len;   
	int fd;
	FILE* fp;
    char rcv_buf[100];         
    char * AT="AT\r"; 
	char * first_4g= "AT^SYSCONFIG=2,6,1,2\r";
	char * query_4g = "AT^SYSINFO\r";
	char * pdp = "AT+CGACT=1,1\r";
	char * rndis = "AT+ZGACT=1,1\r";
	int err;
	fd = UART0_Open(fd,"/dev/ttyUSB0"); //�򿪴��ڣ������ļ�������  
    do{  
                  err = UART0_Init(fd,115200,0,8,1,'N');  
                  printf("Set Port Exactly!\n");  
       }while(FALSE == err || FALSE == fd);
	UART0_Send_Recv(fd,AT);
	UART0_Send_Recv(fd,first_4g);
	UART0_Send_Recv(fd,query_4g);
	UART0_Send_Recv(fd,pdp);
	UART0_Send_Recv(fd,rndis);
    char *buf = "nameserver 114.114.114.114";
    fp  = fopen("/etc/resolv.conf","w");
    fprintf(fp,"\n%s\n",buf);
	system("udhcpc -i eth1");
	sleep(2);
//	system("ping www.qq.com");
//	sleep(5);
	 UART0_Close(fd);
} 

