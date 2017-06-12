###############################################################
#
# Purpose: Makefile for "Adhoc Trans"
# Author.: gaoqing
# Version: 0.1
# License: 
#
###############################################################
SERVER = usbcamera
CC = gcc
#CC = /opt/freescale/usr/local/gcc-4.6.2-glibc-2.13-linaro-multilib-2011.12/fsl-linaro-toolchain/bin/arm-none-linux-gnueabi-g++
OTHER_HEADERS =
LDFLAGS += -static
CXXFLAGS += -g
CXXFLAGS += -DDEBUG
# general compile flags, enable all warnings to make compile more verbose
CXXFLAGS += -O2 -DLINUX -D_GNU_SOURCE -Wall
# we are using the libraries "libpthread" and "libdl"
# libpthread is used to run several tasks (virtually) in parallel
# libdl is used to load the plugins (shared objects) at runtime
LFLAGS += -lpthread -ldl

#如果只有一个目标，一次性编译所有文件可以这么写
#SRCS=$(wildcard *cpp)
#OBJS=$(SRCS:.cpp=.o)
#$(CLIENT): $(OBJS)
#	$(CC) ${CXXFLAGS} $(LFLAGS) $^ -o $@

.PHONY:all
all:$(SERVER)


SRC_SERVER=main.c usb_camera.c
OBJ_SERVER=$(SRC_SERVER:.cpp=.o)

#$(LFLAGS) is necessary
$(SERVER): $(OBJ_SERVER)
	$(CC) $(LDFLAGS) $(OBJ_SERVER) -o $(SERVER) $(LFLAGS)
#compile all .cpp to .o 所有编译相关都写在这里，如-I /usr/....
%o:%cpp 
	$(CC) ${CXXFLAGS}  -c $< -o $@

.PHONY:clean
clean:
	rm -f *.a *.o $(SERVER) core *~ *.so *.lo

