#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>

#define DEVICE_NAME "mydev"
#define BUFFER_SIZE 1024

static int major;
static char kernel_buffer[BUFFER_SIZE];
static int buffer_size = 0;

// OPEN
static int mydev_open(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "mydev: opened\n");
    return 0;
}

// RELEASE
static int mydev_release(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "mydev: closed\n");
    return 0;
}

// READ
static ssize_t mydev_read(struct file *file, char __user *user_buf, size_t len, loff_t *offset)
{
    if (*offset >= buffer_size)
        return 0;

    if (len > buffer_size - *offset)
        len = buffer_size - *offset;

    if (copy_to_user(user_buf, kernel_buffer + *offset, len))
        return -EFAULT;

    *offset += len;
    return len;
}

// WRITE
static ssize_t mydev_write(struct file *file, const char __user *user_buf, size_t len, loff_t *offset)
{
    if (len > BUFFER_SIZE)
        len = BUFFER_SIZE;

    if (copy_from_user(kernel_buffer, user_buf, len))
        return -EFAULT;

    buffer_size = len;

    printk(KERN_INFO "mydev: received %zu bytes\n", len);
    return len;
}

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = mydev_open,
    .release = mydev_release,
    .read = mydev_read,
    .write = mydev_write,
};

// INIT
static int __init mydev_init(void)
{
    major = register_chrdev(0, DEVICE_NAME, &fops);
    if (major < 0) {
        printk(KERN_ERR "Failed to register device\n");
        return major;
    }

    printk(KERN_INFO "mydev registered with major %d\n", major);
    return 0;
}

// EXIT
static void __exit mydev_exit(void)
{
    unregister_chrdev(major, DEVICE_NAME);
    printk(KERN_INFO "mydev unregistered\n");
}

module_init(mydev_init);
module_exit(mydev_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Ban");
MODULE_DESCRIPTION("Simple Char Driver");