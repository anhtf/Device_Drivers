#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/slab.h>

#define DEVICE_NAME "scull0"
#define SCULL_COUNT 1
#define SCULL_BUFFER_SIZE 1024

MODULE_LICENSE("GPL");
MODULE_AUTHOR("OpenAI");
MODULE_DESCRIPTION("Simple scull-like char driver");
MODULE_VERSION("1.0");

static dev_t scull_devno;
static struct cdev scull_cdev;

static char *scull_buffer;
static size_t scull_data_size;

/* open */
static int scull_open(struct inode *inode, struct file *filp)
{
    pr_info("scull: open() called, major=%d minor=%d\n",
            imajor(inode), iminor(inode));
    return 0;
}

/* release */
static int scull_release(struct inode *inode, struct file *filp)
{
    pr_info("scull: release() called\n");
    return 0;
}

/* read */
static ssize_t scull_read(struct file *filp, char __user *buf,
                          size_t count, loff_t *f_pos)
{
    size_t available;
    size_t to_read;

    if (*f_pos >= scull_data_size)
        return 0; /* EOF */

    available = scull_data_size - *f_pos;
    to_read = (count < available) ? count : available;

    if (copy_to_user(buf, scull_buffer + *f_pos, to_read))
        return -EFAULT;

    *f_pos += to_read;

    pr_info("scull: read %zu bytes, f_pos=%lld\n",
            to_read, *f_pos);

    return to_read;
}

/* write */
static ssize_t scull_write(struct file *filp, const char __user *buf,
                           size_t count, loff_t *f_pos)
{
    size_t space_left;
    size_t to_write;

    if (*f_pos >= SCULL_BUFFER_SIZE)
        return -ENOSPC;

    space_left = SCULL_BUFFER_SIZE - *f_pos;
    to_write = (count < space_left) ? count : space_left;

    if (copy_from_user(scull_buffer + *f_pos, buf, to_write))
        return -EFAULT;

    *f_pos += to_write;

    if (*f_pos > scull_data_size)
        scull_data_size = *f_pos;

    pr_info("scull: write %zu bytes, f_pos=%lld, data_size=%zu\n",
            to_write, *f_pos, scull_data_size);

    return to_write;
}

/* optional: reset file position */
static loff_t scull_llseek(struct file *filp, loff_t off, int whence)
{
    loff_t newpos = 0;

    switch (whence) {
    case 0: /* SEEK_SET */
        newpos = off;
        break;
    case 1: /* SEEK_CUR */
        newpos = filp->f_pos + off;
        break;
    case 2: /* SEEK_END */
        newpos = scull_data_size + off;
        break;
    default:
        return -EINVAL;
    }

    if (newpos < 0)
        return -EINVAL;

    if (newpos > SCULL_BUFFER_SIZE)
        newpos = SCULL_BUFFER_SIZE;

    filp->f_pos = newpos;
    return newpos;
}

static const struct file_operations scull_fops = {
    .owner   = THIS_MODULE,
    .open    = scull_open,
    .release = scull_release,
    .read    = scull_read,
    .write   = scull_write,
    .llseek  = scull_llseek,
};

static int __init scull_init(void)
{
    int ret;

    /* 1) cấp phát device number động */
    ret = alloc_chrdev_region(&scull_devno, 0, SCULL_COUNT, "scull");
    if (ret < 0) {
        pr_err("scull: alloc_chrdev_region failed\n");
        return ret;
    }

    pr_info("scull: allocated devno major=%d minor=%d\n",
            MAJOR(scull_devno), MINOR(scull_devno));

    /* 2) cấp phát buffer kernel */
    scull_buffer = kmalloc(SCULL_BUFFER_SIZE, GFP_KERNEL);
    if (!scull_buffer) {
        unregister_chrdev_region(scull_devno, SCULL_COUNT);
        return -ENOMEM;
    }

    memset(scull_buffer, 0, SCULL_BUFFER_SIZE);
    scull_data_size = 0;

    /* 3) init cdev */
    cdev_init(&scull_cdev, &scull_fops);
    scull_cdev.owner = THIS_MODULE;

    /* 4) add cdev vào kernel */
    ret = cdev_add(&scull_cdev, scull_devno, SCULL_COUNT);
    if (ret < 0) {
        pr_err("scull: cdev_add failed\n");
        kfree(scull_buffer);
        unregister_chrdev_region(scull_devno, SCULL_COUNT);
        return ret;
    }

    pr_info("scull: module loaded successfully\n");
    return 0;
}

static void __exit scull_exit(void)
{
    cdev_del(&scull_cdev);
    kfree(scull_buffer);
    unregister_chrdev_region(scull_devno, SCULL_COUNT);
    pr_info("scull: module unloaded\n");
}

module_init(scull_init);
module_exit(scull_exit);