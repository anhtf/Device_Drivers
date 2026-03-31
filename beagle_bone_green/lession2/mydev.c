#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>

#define DEVICE_NAME "mydev"
#define CLASS_NAME  "mydev_class"
#define BUF_SIZE    1024

static dev_t dev_num;
static struct cdev my_cdev;
static struct class *my_class;
static struct device *my_device;

static char kernel_buffer[BUF_SIZE];
static size_t data_size;

static int my_open(struct inode *inode, struct file *file)
{
    pr_info("mydev: open\n");
    return 0;
}

static int my_release(struct inode *inode, struct file *file)
{
    pr_info("mydev: release\n");
    return 0;
}

static ssize_t my_read(struct file *file, char __user *buf, size_t len, loff_t *off)
{
    size_t bytes_to_read;

    if (*off >= data_size)
        return 0;

    bytes_to_read = min(len, data_size - (size_t)*off);

    if (copy_to_user(buf, kernel_buffer + *off, bytes_to_read))
        return -EFAULT;

    *off += bytes_to_read;
    pr_info("mydev: read %zu bytes\n", bytes_to_read);
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

    pr_info("mydev: write %zu bytes: %s\n", bytes_to_write, kernel_buffer);
    return bytes_to_write;
}

static const struct file_operations my_fops = {
    .owner   = THIS_MODULE,
    .open    = my_open,
    .release = my_release,
    .read    = my_read,
    .write   = my_write,
};

static int __init mydev_init(void)
{
    int ret;

    ret = alloc_chrdev_region(&dev_num, 0, 1, DEVICE_NAME);
    if (ret < 0) {
        pr_err("mydev: alloc_chrdev_region failed\n");
        return ret;
    }

    cdev_init(&my_cdev, &my_fops);
    my_cdev.owner = THIS_MODULE;

    ret = cdev_add(&my_cdev, dev_num, 1);
    if (ret < 0) {
        pr_err("mydev: cdev_add failed\n");
        unregister_chrdev_region(dev_num, 1);
        return ret;
    }

    my_class = class_create(CLASS_NAME);
    if (IS_ERR(my_class)) {
        pr_err("mydev: class_create failed\n");
        cdev_del(&my_cdev);
        unregister_chrdev_region(dev_num, 1);
        return PTR_ERR(my_class);
    }

    my_device = device_create(my_class, NULL, dev_num, NULL, DEVICE_NAME);
    if (IS_ERR(my_device)) {
        pr_err("mydev: device_create failed\n");
        class_destroy(my_class);
        cdev_del(&my_cdev);
        unregister_chrdev_region(dev_num, 1);
        return PTR_ERR(my_device);
    }

    pr_info("mydev: loaded, major=%d minor=%d\n", MAJOR(dev_num), MINOR(dev_num));
    return 0;
}

static void __exit mydev_exit(void)
{
    device_destroy(my_class, dev_num);
    class_destroy(my_class);
    cdev_del(&my_cdev);
    unregister_chrdev_region(dev_num, 1);
    pr_info("mydev: unloaded\n");
}

module_init(mydev_init);
module_exit(mydev_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Bao Chau");
MODULE_DESCRIPTION("Lesson 2 simple char device driver");