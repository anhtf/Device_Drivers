#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/kernel.h>

#include "mychar_ioctl.h"

#define DEVICE_NAME "mychar"
#define CLASS_NAME  "mychar_class"
#define DEFAULT_BUF_SIZE 1024

struct mychar_dev {
    dev_t dev_num;
    struct cdev cdev;
    struct class *class;
    struct device *device;

    char *buffer;
    size_t buf_size;      /* total capacity */
    size_t data_size;     /* actual valid data */
    struct mutex lock;
};

static struct mychar_dev mydev;

static int mychar_open(struct inode *inode, struct file *file)
{
    file->private_data = &mydev;
    pr_info("mychar: open\n");
    return 0;
}

static int mychar_release(struct inode *inode, struct file *file)
{
    pr_info("mychar: release\n");
    return 0;
}

static ssize_t mychar_read(struct file *file, char __user *buf, size_t len, loff_t *ppos)
{
    struct mychar_dev *dev = file->private_data;
    size_t bytes_to_read;
    int ret;

    if (mutex_lock_interruptible(&dev->lock))
        return -ERESTARTSYS;

    if (*ppos >= dev->data_size) {
        mutex_unlock(&dev->lock);
        return 0;
    }

    bytes_to_read = min(len, dev->data_size - (size_t)*ppos);

    ret = copy_to_user(buf, dev->buffer + *ppos, bytes_to_read);
    if (ret) {
        mutex_unlock(&dev->lock);
        return -EFAULT;
    }

    *ppos += bytes_to_read;

    pr_info("mychar: read %zu bytes, new offset=%lld\n",
            bytes_to_read, *ppos);

    mutex_unlock(&dev->lock);
    return bytes_to_read;
}

static ssize_t mychar_write(struct file *file, const char __user *buf, size_t len, loff_t *ppos)
{
    struct mychar_dev *dev = file->private_data;
    size_t bytes_to_write;
    int ret;

    if (mutex_lock_interruptible(&dev->lock))
        return -ERESTARTSYS;

    if (*ppos >= dev->buf_size) {
        mutex_unlock(&dev->lock);
        return -ENOSPC;
    }

    bytes_to_write = min(len, dev->buf_size - (size_t)*ppos);

    ret = copy_from_user(dev->buffer + *ppos, buf, bytes_to_write);
    if (ret) {
        mutex_unlock(&dev->lock);
        return -EFAULT;
    }

    *ppos += bytes_to_write;

    if (*ppos > dev->data_size)
        dev->data_size = *ppos;

    pr_info("mychar: write %zu bytes, new offset=%lld, data_size=%zu\n",
            bytes_to_write, *ppos, dev->data_size);

    mutex_unlock(&dev->lock);
    return bytes_to_write;
}

static loff_t mychar_llseek(struct file *file, loff_t offset, int whence)
{
    struct mychar_dev *dev = file->private_data;
    loff_t newpos;

    mutex_lock(&dev->lock);

    switch (whence) {
    case SEEK_SET:
        newpos = offset;
        break;
    case SEEK_CUR:
        newpos = file->f_pos + offset;
        break;
    case SEEK_END:
        newpos = dev->data_size + offset;
        break;
    default:
        mutex_unlock(&dev->lock);
        return -EINVAL;
    }

    if (newpos < 0 || newpos > dev->buf_size) {
        mutex_unlock(&dev->lock);
        return -EINVAL;
    }

    file->f_pos = newpos;
    mutex_unlock(&dev->lock);

    pr_info("mychar: llseek -> %lld\n", newpos);
    return newpos;
}

static long mychar_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    struct mychar_dev *dev = file->private_data;
    int value;

    if (_IOC_TYPE(cmd) != MYCHAR_MAGIC)
        return -EINVAL;

    if (mutex_lock_interruptible(&dev->lock))
        return -ERESTARTSYS;

    switch (cmd) {
    case MYCHAR_IOCTL_CLEAR:
        memset(dev->buffer, 0, dev->buf_size);
        dev->data_size = 0;
        pr_info("mychar: buffer cleared\n");
        break;

    case MYCHAR_IOCTL_GET_SIZE:
        value = dev->data_size;
        if (copy_to_user((int __user *)arg, &value, sizeof(value))) {
            mutex_unlock(&dev->lock);
            return -EFAULT;
        }
        pr_info("mychar: get size = %d\n", value);
        break;

    case MYCHAR_IOCTL_SET_LIMIT:
        if (copy_from_user(&value, (int __user *)arg, sizeof(value))) {
            mutex_unlock(&dev->lock);
            return -EFAULT;
        }

        if (value <= 0 || value > DEFAULT_BUF_SIZE) {
            mutex_unlock(&dev->lock);
            return -EINVAL;
        }

        dev->buf_size = value;
        if (dev->data_size > dev->buf_size)
            dev->data_size = dev->buf_size;

        pr_info("mychar: new limit = %d\n", value);
        break;

    default:
        mutex_unlock(&dev->lock);
        return -EINVAL;
    }

    mutex_unlock(&dev->lock);
    return 0;
}

static const struct file_operations mychar_fops = {
    .owner          = THIS_MODULE,
    .open           = mychar_open,
    .release        = mychar_release,
    .read           = mychar_read,
    .write          = mychar_write,
    .llseek         = mychar_llseek,
    .unlocked_ioctl = mychar_ioctl,
};

static int __init mychar_init(void)
{
    int ret;

    mydev.buf_size = DEFAULT_BUF_SIZE;
    mydev.data_size = 0;

    mutex_init(&mydev.lock);

    mydev.buffer = kzalloc(DEFAULT_BUF_SIZE, GFP_KERNEL);
    if (!mydev.buffer)
        return -ENOMEM;

    ret = alloc_chrdev_region(&mydev.dev_num, 0, 1, DEVICE_NAME);
    if (ret < 0) {
        pr_err("mychar: alloc_chrdev_region failed\n");
        kfree(mydev.buffer);
        return ret;
    }

    cdev_init(&mydev.cdev, &mychar_fops);
    mydev.cdev.owner = THIS_MODULE;

    ret = cdev_add(&mydev.cdev, mydev.dev_num, 1);
    if (ret < 0) {
        pr_err("mychar: cdev_add failed\n");
        unregister_chrdev_region(mydev.dev_num, 1);
        kfree(mydev.buffer);
        return ret;
    }

    mydev.class = class_create(CLASS_NAME);
    if (IS_ERR(mydev.class)) {
        pr_err("mychar: class_create failed\n");
        cdev_del(&mydev.cdev);
        unregister_chrdev_region(mydev.dev_num, 1);
        kfree(mydev.buffer);
        return PTR_ERR(mydev.class);
    }

    mydev.device = device_create(mydev.class, NULL, mydev.dev_num, NULL, DEVICE_NAME);
    if (IS_ERR(mydev.device)) {
        pr_err("mychar: device_create failed\n");
        class_destroy(mydev.class);
        cdev_del(&mydev.cdev);
        unregister_chrdev_region(mydev.dev_num, 1);
        kfree(mydev.buffer);
        return PTR_ERR(mydev.device);
    }

    pr_info("mychar: loaded major=%d minor=%d buf_size=%zu\n",
            MAJOR(mydev.dev_num), MINOR(mydev.dev_num), mydev.buf_size);
    return 0;
}

static void __exit mychar_exit(void)
{
    device_destroy(mydev.class, mydev.dev_num);
    class_destroy(mydev.class);
    cdev_del(&mydev.cdev);
    unregister_chrdev_region(mydev.dev_num, 1);
    kfree(mydev.buffer);
    pr_info("mychar: unloaded\n");
}

module_init(mychar_init);
module_exit(mychar_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Bao Chau");
MODULE_DESCRIPTION("Lesson 4 complete char driver");