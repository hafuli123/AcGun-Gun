#ifndef __DEV_H__
#define __DEV_H__

#include "main.h"
#include <stdarg.h>

#define DEVICE_LIST_NUM     10

typedef struct __device
{
    char device_name[10];
    uint8_t (*open)(const char *pathname,int flags);
    uint8_t (*read)(int fd,void*buf,int count);
    uint8_t (*write)(int fd,void*buf,int count);
    uint8_t (*ioctl)(int fd, void *cmd1, void *cmd2);
}device;

typedef struct __device_list
{
    device *DeviceList[DEVICE_LIST_NUM];
    int Device_List_index;
}device_list;

void Device_register(device *mydev);
int Device_open(const char *pathname, int flags);
uint8_t Device_read(int fd,void*buf,int count);
uint8_t Device_write(int fd,void*buf,int count);
uint8_t Device_ioctl(int fd, void *cmd1, void *cmd2);
#endif
