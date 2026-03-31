#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/string.h>

#include "mydev_ioctl.h"

#define DEVICE_NAME "mydev_ioctl"
#define CLASS_NAME  "mydev_ioctl_class"
#define BUF_SIZE    1024

static dev_t dev_num;
static struct cdev my_cdev;
static struct class *my_class;
static struct device *my_device;

static char kernel_buffer[BUF_SIZE];
static int data_size = 0;

static int my_open(struct inode *inode, struct file *file)
{
    pr_info("mydev_ioctl: open\n");
    return 0;
}

static int my_release(struct inode *inode, struct file *file)
{
    pr_info("mydev_ioctl: release\n");
    return 0;
}

static ssize_t my_read(struct file *file, char __user *buf, size_t len, loff_t *off)
{
    size_t bytes_to_read;

    if (*off >= data_size)
        return 0;

    bytes_to_read = min(len, (size_t)(data_size - *off));

    if (copy_to_user(buf, kernel_buffer + *off, bytes_to_read))
        return -EFAULT;

    *off += bytes_to_read;

    pr_info("mydev_ioctl: read %zu bytes\n", bytes_to_read);
    return bytes_to_read;
}

static ssize_t my_write(struct file *file, const char __user *buf, size_t len, loff_t *off)
{
    size_t bytes_to_write;

    bytes_to_write = min(len, (size_t)(BUF_SIZE - 1));

    if (copy_from_user(kernel_buffer, buf, bytes_to_write))
        return -EFAULT;

    kernel_buffer[bytes_to_write] = '\0';
    data_size = bytes_to_write;

    pr_info("mydev_ioctl: write %zu bytes: %s\n", bytes_to_write, kernel_buffer);
    return bytes_to_write;
}

static long my_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    int size;
    char temp_buf[BUF_SIZE];

    switch (cmd) {
    case IOCTL_CLEAR_BUFFER:
        memset(kernel_buffer, 0, BUF_SIZE);
        data_size = 0;
        pr_info("mydev_ioctl: buffer cleared\n");
        break;

    case IOCTL_GET_SIZE:
        size = data_size;
        if (copy_to_user((int __user *)arg, &size, sizeof(size)))
            return -EFAULT;
        pr_info("mydev_ioctl: get size = %d\n", size);
        break;

    case IOCTL_SET_MSG:
        if (copy_from_user(temp_buf, (char __user *)arg, BUF_SIZE - 1))
            return -EFAULT;

        temp_buf[BUF_SIZE - 1] = '\0';
        strncpy(kernel_buffer, temp_buf, BUF_SIZE - 1);
        kernel_buffer[BUF_SIZE - 1] = '\0';
        data_size = strlen(kernel_buffer);

        pr_info("mydev_ioctl: set msg = %s\n", kernel_buffer);
        break;

    default:
        pr_err("mydev_ioctl: invalid ioctl cmd\n");
        return -EINVAL;
    }

    return 0;
}

static const struct file_operations my_fops = {
    .owner          = THIS_MODULE,
    .open           = my_open,
    .release        = my_release,
    .read           = my_read,
    .write          = my_write,
    .unlocked_ioctl = my_ioctl,
};

static int __init mydev_init(void)
{
    int ret;

    ret = alloc_chrdev_region(&dev_num, 0, 1, DEVICE_NAME);
    if (ret < 0) {
        pr_err("mydev_ioctl: alloc_chrdev_region failed\n");
        return ret;
    }

    cdev_init(&my_cdev, &my_fops);
    my_cdev.owner = THIS_MODULE;

    ret = cdev_add(&my_cdev, dev_num, 1);
    if (ret < 0) {
        pr_err("mydev_ioctl: cdev_add failed\n");
        unregister_chrdev_region(dev_num, 1);
        return ret;
    }

    my_class = class_create(CLASS_NAME);
    if (IS_ERR(my_class)) {
        pr_err("mydev_ioctl: class_create failed\n");
        cdev_del(&my_cdev);
        unregister_chrdev_region(dev_num, 1);
        return PTR_ERR(my_class);
    }

    my_device = device_create(my_class, NULL, dev_num, NULL, DEVICE_NAME);
    if (IS_ERR(my_device)) {
        pr_err("mydev_ioctl: device_create failed\n");
        class_destroy(my_class);
        cdev_del(&my_cdev);
        unregister_chrdev_region(dev_num, 1);
        return PTR_ERR(my_device);
    }

    pr_info("mydev_ioctl: loaded major=%d minor=%d\n",
            MAJOR(dev_num), MINOR(dev_num));
    return 0;
}

static void __exit mydev_exit(void)
{
    device_destroy(my_class, dev_num);
    class_destroy(my_class);
    cdev_del(&my_cdev);
    unregister_chrdev_region(dev_num, 1);
    pr_info("mydev_ioctl: unloaded\n");
}

module_init(mydev_init);
module_exit(mydev_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Bao Chau");
MODULE_DESCRIPTION("Lesson 3 char device with ioctl");