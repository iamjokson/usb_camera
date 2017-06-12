#ifndef __CAMERA__
#define __CAMERA__

int init_camera(int width, int height, char *dev);
void Grep(char *filename,int fd);
int start_cap_mjpg(unsigned char* buf);
void stop_cap_mjpg();
void uninit_camera();
void send_picture(int fd, int size);
int getframe();
void  SaveSd();
void RmFile();
void See(int fd);
void RmFile(char *filename);
int getfps();
int set_backlight_compensation(int compensation);
int get_backlight_compensation();
#endif
