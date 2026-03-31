#ifndef MYCHAR_IOCTL_H
#define MYCHAR_IOCTL_H

#include <linux/ioctl.h>

#define MYCHAR_MAGIC 'M'

#define MYCHAR_IOCTL_CLEAR     _IO(MYCHAR_MAGIC, 0)
#define MYCHAR_IOCTL_GET_SIZE  _IOR(MYCHAR_MAGIC, 1, int)
#define MYCHAR_IOCTL_SET_LIMIT _IOW(MYCHAR_MAGIC, 2, int)

#endif