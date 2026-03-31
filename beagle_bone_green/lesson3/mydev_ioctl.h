#ifndef MYDEV_IOCTL_H
#define MYDEV_IOCTL_H

#include <linux/ioctl.h>

#define MYDEV_MAGIC 'k'

#define IOCTL_CLEAR_BUFFER _IO(MYDEV_MAGIC, 0)
#define IOCTL_GET_SIZE     _IOR(MYDEV_MAGIC, 1, int)
#define IOCTL_SET_MSG      _IOW(MYDEV_MAGIC, 2, char *)

#endif