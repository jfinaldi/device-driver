/**************************************************************
* Class:  CSC-415-02 FALL 2020
* Name: Jennifer Finaldi
* Student ID: 920290420
* Project: Assignment 6 – Device Driver
*
* File: dongle.c
*
* Description: A character driver to implement basic 
*       functionality of a 2 factor authentication dongle
*       that will generate a temporary login token for the user
*       to enter into the shell to "login" to a generic
*       service.
*
**************************************************************/
#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>

#define DEVICE_NAME "dongle"
#define CLASS_NAME "dgl"

MODULE_LICENSE("DGL");
MODULE_AUTHOR("Jennifer Finaldi");
MODULE_DESCRIPTION("A simple 2 factor authentication dongle");
MODULE_VERSION("1.0");

static int majorNumber;                //The device number
static char message[256] = {0};        //stores a user-passed string  (MAYBE USELESS)
static short sizeOfMessage;            //stores size of string    (MAYBE USELESS)
static int numberOpens = 0;            //# of times device opened (MAYBE USELESS)
static struct class* dglClass = NULL;  //device driver class struct pointer
static struct device* dglDevice = NULL;//device driver device struct pointer    

//prototypes for char driver
static int dgl_open(struct inode*, struct file*);
static int dgl_ioctl(struct inode*, struct file*, unsigned int cmd, unsigned long data);//MIGHT REVISE THIS
static int dgl_release(struct inode*, struct file*);
static ssize_t dgl_read(struct file*, char*, size_t, loff_t*);
static ssize_t dgl_write(struct file*, const char*, size_t, loff_t*);

//hooks
static struct file_operations fops =
{
    .open = dgl_open,
    .release = dgl_release,
    .read = dgl_read,
    .write = dgl_write,
    .ioctl = dgl_ioctl,
};

//init function
static int __init dongle_init(void) {
    printk(KERN_INFO "Dongle: Initializing Dongle Linux Kernel Module\n");

    //dynamically allocated major number
    //majorNumber = register_chrdev(0, DEVICE_NAME, &fops);
    if((majorNumber = register_chrdev(0, DEVICE_NAME, &fops))) {
        printk(KERN_ALERT "Dongle major number registration failed, producing %d\n", majorNumber);
        return majorNumber;
    } 
    else {
        printk(KERN_INFO "Dongle major number successfuly registered wiith %d\n", majorNumber)
    }

    //register device class
    dglClass = class_create(THIS_MODULE, CLASS_NAME);
    if(IS_ERR(dglClass)) {
        unregister_chrdev(majorNumber, DEVICE_NAME);
        printk(KERN_ALERT "Error registering device class\n");
        return PTR_ERR(dglClass);
    }
    else {
        printk(KERN_INFO "Dongle class successfully registered\n");
    }

    //register device driver
    dglDevice = device_create(dglClass, NULL, MKDEV(majorNumber, 0), NULL, DEVICE_NAME);
    if(IS_ERR(dglDevice)) {
        class_destroy(dglClass);
        unregister_chrdev(majorNumber, DEVICE_NAME);
        printk(KERN_ALERT "Dongle driver registration failed\n");
        return PTR_ERR(dglDevice);
    }
    else {
        printk(KERN_INFO "Dongle driver successfully registered\n");
    }

    printk(KERN_INFO "Dongle device successfully initialized\n");
    return 0;

}

static void __exit dgl_exit(void) {
    device_detroy(dglClass, MKDEV(majorNumber, 0));
    class_unregister(dglClass);
    class_destroy(dglClass);
    unregister_chrdev(majorNumber, DEVICE_NAME);
    printk(KERN_INFO "Dongle successfully removed from the system\n");
}

static int dgl_open(struct inode* inp, struct file* fp) {
    int x = alloc_dongle
    return 0;
}

static int dgl_release(struct inode* inp, struct file* fp) {
    return 0;
}

static ssize_t dgl_read(struct file* fp, char* buf, size_t lenbuf, loff_t* off) {
    //Trigger ioctl to read user input for either "generate" or the given key
}

static ssize_t dgl_write(struct file* fp, const char* buf, size_t lenbuf, loff_t* off) {
    //Perhaps this can toggle the ioctl to generate a random key
}


