// jiffies_driver.c
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/jiffies.h>

#define DEVICE_NAME "jiffies_dev"

static int major;
static char msg[64];

// open
static int dev_open(struct inode *inode, struct file *file)
{
    return 0;
}

// read
static ssize_t dev_read(struct file *file, char __user *buffer,
                        size_t len, loff_t *offset)
{
    int msg_len;

    if (*offset > 0)
        return 0; // EOF

    msg_len = snprintf(msg, sizeof(msg), "jiffies = %lu\n", jiffies);

    if (copy_to_user(buffer, msg, msg_len))
        return -EFAULT;

    *offset = msg_len;
    return msg_len;
}

// close
static int dev_release(struct inode *inode, struct file *file)
{
    return 0;
}

// file operations
static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = dev_open,
    .read = dev_read,
    .release = dev_release,
};

// init
static int __init jiffies_init(void)
{
    major = register_chrdev(0, DEVICE_NAME, &fops);
    printk("jiffies driver loaded, major = %d\n", major);
    return 0;
}

// exit
static void __exit jiffies_exit(void)
{
    unregister_chrdev(major, DEVICE_NAME);
    printk("jiffies driver unloaded\n");
}

module_init(jiffies_init);
module_exit(jiffies_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Ban");
MODULE_DESCRIPTION("Jiffies Char Driver");