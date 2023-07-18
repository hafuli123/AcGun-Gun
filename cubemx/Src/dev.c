#include "dev.h"
#include "stm32f4xx_hal.h"
#include <rtthread.h>
#include <string.h>
device_list global_dev_list;

void Device_register(device *mydev)
{
    int index = global_dev_list.Device_List_index;
    global_dev_list.DeviceList[index]=mydev;
    index++;
    global_dev_list.Device_List_index = index;
}

int Device_find(const char*device_name)
{
    int i;
    int index = global_dev_list.Device_List_index;
    for(i=0;i<index;i++){
        if( strcmp(global_dev_list.DeviceList[i]->device_name , device_name)==0 ){
            return i;
        }
    }
    return DEVICE_LIST_NUM;
}

int Device_open(const char *pathname, int flags)
{
    int fd;
    device *dev;
    fd=Device_find(pathname);
    dev = global_dev_list.DeviceList[fd];
    dev->open(pathname,flags);
    return fd;
}

uint8_t Device_read(int fd,void*buf,int count)
{
    device *dev;
    dev = global_dev_list.DeviceList[fd];
    return dev->read(fd,buf,count);
}

uint8_t Device_write(int fd,void*buf,int count)
{
    device *dev;
    dev = global_dev_list.DeviceList[fd];
    return dev->write(fd,buf,count);
}

uint8_t Device_ioctl(int fd, void *cmd1, void *cmd2)
{
    device *dev;
    dev = global_dev_list.DeviceList[fd];
    return dev->ioctl(fd,cmd1,cmd2);
}
