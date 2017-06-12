#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include<pthread.h>

#include <fcntl.h>              /* low-level i/o */
#include <unistd.h>
#include <errno.h>
#include <malloc.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#include <asm/types.h>          /* for videodev2.h */
#include <linux/videodev2.h>
#include "usb_camera.h"

struct buffer {
    void * start;
    unsigned int length;
};

char dev_name[50];	
int fd = -1;
pthread_mutex_t lock;
struct buffer * buffers=NULL;
unsigned int n_buffers = 0;
int capstate = 0;
int fps = 0;
unsigned char* cambuf = NULL;
void* framebuf = NULL;
int camsize = 0;
pthread_t id_cap;


/** 
      * 计算两个时间的间隔，得到时间差 
      * @param struct timeval* resule 返回计算出来的时间 
      * @param struct timeval* x 需要计算的前一个时间 
      * @param struct timeval* y 需要计算的后一个时间 
      * return -1 failure ,0 success 
  **/ 
int timeval_subtract(struct timeval* result, struct timeval* x, struct timeval* y) 
  { 
        if ( x->tv_sec>y->tv_sec ) 
                  return -1; 
    
        if ( (x->tv_sec==y->tv_sec) && (x->tv_usec>y->tv_usec) ) 
                  return -1; 
    
        result->tv_sec = ( y->tv_sec-x->tv_sec ); 
        result->tv_usec = ( y->tv_usec-x->tv_usec ); 
    
        if (result->tv_usec<0) 
        { 
                  result->tv_sec--; 
                  result->tv_usec+=1000000; 
        } 
    
        return 0; 
  }

int xioctl(int fd, int IOCTL_X, void * arg) {
	int ret;
	int tries = 4;
	do {
		ret = ioctl(fd, IOCTL_X, arg);
	} while (-1 == ret &&tries--&& EINTR == errno);
	if(-1==ret && (tries <= 0)) 
		printf("ioctl %d retried %i times - giving up: %s)\n", IOCTL_X, 4, strerror(errno));
	return ret;
}


int read_frame() {
	struct v4l2_buffer buf;
	memset(&buf, 0, sizeof(buf));
	
	buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	buf.memory = V4L2_MEMORY_MMAP;

	if (-1 == xioctl(fd, VIDIOC_DQBUF, &buf)) {//从队列中取数据到缓冲区
		printf("VIDIOC_DQBUF error %d , %s\n", errno, strerror(errno));
	}
	assert(buf.index < n_buffers);

	if(buf.bytesused <= 0xaf) {
            /* Prevent crash on empty image */
            printf("Ignoring empty buffer ...\n");
            return -1;
        }

	pthread_mutex_lock(&lock);
	camsize = buf.length; //YUV:buf.length ,MJPEG: buf.bytesused;
	memcpy(framebuf,buffers[buf.index].start, buf.bytesused);
	pthread_mutex_unlock(&lock);
	if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))//填充队列
		printf("VIDIOC_QBUF error %d, %s", errno, strerror(errno));
	return 0;
}

void cap_stremloop() {
    struct timeval start_time, over_time, consume_time;
    int mfps = 0;
    int ret;
    
    fd_set fds;
    struct timeval tv;
    gettimeofday(&over_time, NULL);//get the current time
    start_time = over_time;

    while(capstate) {	
         /* Timeout. */
        tv.tv_sec = 2;
        tv.tv_usec = 0;
        FD_ZERO(&fds);
        FD_SET(fd, &fds);
        ret = select(fd + 1, &fds, NULL, NULL, &tv);

        if (-1 == ret) {
        	if (EINTR == errno)
        		continue;
        	printf("capture stream select error %d, %s\n", errno, strerror(errno));
        }

        if (0 == ret) {
        	printf("select timeout :2 s \n");
        }
        read_frame();
        mfps ++;    //计算帧率
        gettimeofday(&over_time, NULL);
        timeval_subtract(&consume_time, &start_time, &over_time);
        if (((consume_time.tv_sec*1000000 + consume_time.tv_usec) >= 1000000) && (mfps > 1)) {	
         //   printf("%s %d fps\n", __FUNCTION__, mfps);
            start_time = over_time;
            fps = mfps;
            mfps = 0;
        }
        /* EAGAIN - continue select loop. */
    }
}

void* cap_stremloop_thread(void* arg){
    cap_stremloop();
    return NULL;
}

void init_capture(void) {
	unsigned int i;
	enum v4l2_buf_type type;
	for (i = 0; i < n_buffers; ++i) {
		struct v4l2_buffer buf;
		memset(&buf, 0, sizeof(buf));
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index = i;

		if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))
			printf("VIDIOC_QBUF error %d, %s\n", errno, strerror(errno));
	}

	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	if (-1 == xioctl(fd, VIDIOC_STREAMON, &type))
		printf("VIDIOC_STREAMON error %d, %s\n", errno, strerror(errno));
}

/*开始视频捕获，传入buffer用于存储图像,buf大小建议200k*/
int start_cap_mjpg(unsigned char* buf)	
{
	if(fd==-1){//打开设备失败
	      printf("device is not open\n");
		return -1;
	}
	capstate = 1;//start get stream flag. if capstate == 0 ,stop gain stream
	init_capture();
      cambuf = buf;
	if(pthread_create(&id_cap,NULL, cap_stremloop_thread,NULL)!=0){
        printf("pthread start cap mjpeg failed");
        return -1;
     }
	pthread_detach(id_cap);
	return 0;
}

/*共享4个v4l设备buffer到用户空间供读取*/
int init_mmap(void) {
	struct v4l2_requestbuffers req;

	memset(&req, 0, sizeof(req));

	req.count = 4;
	req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory = V4L2_MEMORY_MMAP;

	if (-1 == xioctl(fd, VIDIOC_REQBUFS, &req)) {
		if (EINVAL == errno) {
			printf("%s does not support memory mapping\n", dev_name);
		} else {
				printf("VIDIOC_REQBUFS error %d, %s/n", errno, strerror(errno));
		}
	}

	if (req.count < 2) {
		printf("Insufficient buffer memory on %s\n", dev_name);
	}

	buffers = (struct buffer*)calloc(req.count, sizeof(*buffers));

	if (!buffers) {
		printf("calloc memory failed\n");
		return -1;
	}

	for (n_buffers = 0; n_buffers < req.count; ++n_buffers) {
		struct v4l2_buffer buf;
		memset(&buf, 0, sizeof(buf));

		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index = n_buffers;

		if (-1 == xioctl(fd, VIDIOC_QUERYBUF, &buf))
			printf("VIDIOC_QUERYBUF error %d, %s/n", errno, strerror(errno));

		buffers[n_buffers].length = buf.length;
		buffers[n_buffers].start = mmap(NULL /* start anywhere */, buf.length,
				PROT_READ | PROT_WRITE /* required */,
				MAP_SHARED /* recommended */, fd, buf.m.offset);

		if (MAP_FAILED == buffers[n_buffers].start)
			printf("mmap error %d, %s/n", errno, strerror(errno));
	
	}
	return 0;
}

int init_device(int pixwidth, int pixheight) {
	struct v4l2_capability cap;
	struct v4l2_cropcap cropcap;
	struct v4l2_crop crop;
	struct v4l2_format fmt;
	struct v4l2_frmsizeenum frmsize;
	unsigned int min;
	//查询视频设备驱动的功能
	if (-1 == xioctl(fd, VIDIOC_QUERYCAP, &cap)) {
		if (EINVAL == errno) {
			printf("%s is not V4L2 device/n", dev_name);
		} else {
			printf("VIDIOC_QUERYCAP error %d, %s/n", errno, strerror(errno));
		}
		return -1;
	}
	//判断是否是一个视频捕捉设备
	if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
		printf("%s is not video capture device/n", dev_name);
		return -1;
	}
	//判断是否支持视频流形式
	if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
		printf("%s does not support streaming i/o/n", dev_name);
		return -1;
	}
	
	/* Select video input, video standard and tune here. */
	memset(&cropcap, 0, sizeof(cropcap));
	cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (0 == xioctl(fd, VIDIOC_CROPCAP, &cropcap)) {
		crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		crop.c = cropcap.defrect; /* reset to default */

		if (-1 == xioctl(fd, VIDIOC_S_CROP, &crop)) {
			switch (errno) {
			case EINVAL:
				/* Cropping not supported. */
				break;
			default:
				/* Errors ignored. */
				break;
			}
		}
	} else {
		/* Errors ignored. */
	}

	//查看支持的分辨率
	memset(&frmsize, 0, sizeof(struct v4l2_frmsizeenum));
	int index = 0;
	int ret = 0;
	while(ret== 0){
		frmsize.pixel_format = V4L2_PIX_FMT_YUYV;
		frmsize.index = index++;
		ret = ioctl(fd,VIDIOC_ENUM_FRAMESIZES,&frmsize);
		printf("resolution:%d x %d\n", frmsize.discrete.width, frmsize.discrete.height);
	}
	
	
	//设置摄像头采集数据格式，如设置采集数据的
	//长,宽，图像格式(JPEG,YUYV,MJPEG等格式)
	memset(&fmt, 0, sizeof(fmt));
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt.fmt.pix.width = pixwidth;
	fmt.fmt.pix.height = pixheight;
	fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
	fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;

	if (-1 == xioctl(fd, VIDIOC_S_FMT, &fmt))
		printf("VIDIOC_S_FMT error %d, %s\n", errno, strerror(errno));
	/* Note VIDIOC_S_FMT may change width and height. */
	
	/* Buggy driver paranoia. */
	min = fmt.fmt.pix.width * 2;
	if (fmt.fmt.pix.bytesperline < min)
		fmt.fmt.pix.bytesperline = min;
	min = fmt.fmt.pix.bytesperline * fmt.fmt.pix.height;
	if (fmt.fmt.pix.sizeimage < min)
		fmt.fmt.pix.sizeimage = min;
	//初始化视频采集方式(mmap)
	if(init_mmap()!=0)
		return -1;
	return 0;
}



int open_device(void) {
	struct stat st;

	if (-1 == stat(dev_name, &st)) {
		printf("Cannot identify '%s': %d, %s\n", dev_name, errno,strerror(errno));
		return -1;
	}

	if (!S_ISCHR(st.st_mode)) {
		printf("%s is not video device/n", dev_name);
		return -1;
	}

	fd = open(dev_name, O_RDWR /* required */| O_NONBLOCK, 0);

	if (-1 == fd) {
		printf("Cannot open '%s': %d, %s/n", dev_name, errno,strerror(errno));
		return -1;
	}
	return 0;
}

/*初始化camera*/
int init_camera(int width, int height, char *dev) {	
	memcpy(dev_name, dev, 50);
	pthread_mutex_init(&lock,NULL);  
	if(open_device()!=0){
			printf("init_camera :open_device failed\n");
			return -1;
	}
	if(init_device(width, height)!=0){
		printf("init_camera: init_device failed\n");
		return -1;
	}
       //创建frame存储buffer
    	framebuf = (void*)malloc(204800);
	if(!framebuf){
		printf("malloc framebuf failed");
		return -1;
	}
	return 0;
}

/*停止视频捕获*/
void stop_cap_mjpg() {
    if(fd == -1)
    	return ;
    enum v4l2_buf_type type;
    capstate = 0;
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (-1 == xioctl(fd, VIDIOC_STREAMOFF, &type))
    	printf("VIDIOC_STREAMOFF error %d, %s", errno, strerror(errno));
    //停止capture后不能获取到frame fps
    camsize = 0;
    fps = 0;
}

void uninit_device(void) {
	if( fd == -1)
		return;
	unsigned int i;
	for (i = 0; i < n_buffers; ++i)
		if (-1 == munmap(buffers[i].start, buffers[i].length))
			printf("munmap error %d , %s", errno, strerror(errno));
	free(buffers);
}

void close_device(void) {
	if (-1 == close(fd))
		printf("close camera fd error %d,%s", errno, strerror(errno));
	fd = -1;
}

void uninit_camera()
{
	uninit_device();
	close_device();
       //释放frame buf
    	free(framebuf);
	framebuf = NULL;
	cambuf = NULL;
}

/*获取帧率*/
int getfps()
{
	return fps;
}

/*获取一帧图像到start_cap_mjpg()传入的buf中，return: frame size*/
int getframe()
{
	int size = 0;
	if (!framebuf){
		return 0;
	}
	pthread_mutex_lock(&lock);
	size = camsize;
	memcpy(cambuf, framebuf, camsize);
	camsize = 0;
	pthread_mutex_unlock(&lock);
	return size;
}
 /*设置背光补偿等级*/
int set_backlight_compensation(int compensation)
{
	struct v4l2_control ctrl;
	ctrl.id = V4L2_CID_BACKLIGHT_COMPENSATION;
	ctrl.value = compensation;
	if(-1==xioctl(fd, VIDIOC_S_CTRL, &ctrl)){
		printf("set backlight compensation error %d, %s\n", errno, strerror(errno));
		return -1;
	}
	return 0;
}
/*获取背光补偿等级*/
int get_backlight_compensation()
{
	struct v4l2_control ctrl;
	ctrl.id = V4L2_CID_BACKLIGHT_COMPENSATION;
	if(-1==xioctl(fd, VIDIOC_G_CTRL, &ctrl)){
		printf("get backlight compensation error %d, %s\n", errno, strerror(errno));
		return -1;
	}
	return (int)ctrl.value;	
}
