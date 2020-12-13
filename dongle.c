/**************************************************************
* Class:  CSC-415-02 FALL 2020
* Name: Jennifer Finaldi
* Student ID: 920290420
* Project: Assignment 6 – Device Driver
*
* File: dongle.c
*
* Description: A character driver module to implement basic 
*       functionality of a 2 factor authentication dongle
*       that will generate a temporary login token for the user
*       to enter into the shell to "login" to a generic
*       service. 
*
*       Some code and code structure attributed to Derek Molloy:
*       http://derekmolloy.ie/writing-a-linux-kernel-module-part-
*       2-a-character-device/
*   
**************************************************************/
#include <linux/init.h>
#include <linux/ioctl.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/timer.h>
#include <linux/random.h>
#include <linux/string.h>

#define DEVICE_NAME "dongle"
#define CLASS_NAME "don"
#define MAX_KEY_VALUE 99999999
#define MIN_KEY_VALUE 1000000
#define MAX_TIME_INTERVAL 180000 //3 minutes

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jennifer Finaldi");
MODULE_DESCRIPTION("A simple 2 factor authentication dongle");
MODULE_VERSION("1.0");

static int majorNumber;                //The device number
static char message[256] = {0};        //stores a user-passed string 
static short sizeOfMessage;            //stores size of string 
static int error_count;                //stores ret value from copy_to_user   
static int key;                        //stores a random generated key for user to input
static int res;                        //stores a random result value
static int userEnteredKey;             //stores a user entered key to compare to actual key
static int successFailCode;            //0 = success, 1 = fail, 2 = out of time
static int isTimerUp;                  //0 = no, 1 = yes
static struct timer_list myTimer;      //stores timer for valid key time
static struct class* dglClass = NULL;  //device driver class struct pointer
static struct device* dglDevice = NULL;//device driver device struct pointer    

//prototypes for char driver
static int dgl_open(struct inode*, struct file*);
static int dgl_release(struct inode*, struct file*);
static ssize_t dgl_read(struct file*, char*, size_t, loff_t*);
static ssize_t dgl_write(struct file*, const char*, size_t, loff_t*);
static long dgl_ioctl(struct file*, unsigned int cmd, unsigned long data);
static void dgl_generate(void);
static void dgl_start_timer(void);
static void dgl_time_up(struct timer_list* data);
static int dgl_is_correct_key(void);


//hooks
static struct file_operations fops =
{
    .open = dgl_open,
    .read = dgl_read,
    .write = dgl_write,
    .release = dgl_release,
    .unlocked_ioctl = dgl_ioctl,
};

//init function courtesy DerekMolloy.ie
static int __init dongle_init(void) {
    printk(KERN_INFO "Dongle: Initializing Dongle Linux Kernel Module\n");

    //dynamically allocated major number
    majorNumber = register_chrdev(0, DEVICE_NAME, &fops);
    if(majorNumber < 0) {
        printk(KERN_ALERT "Dongle major number registration failed, producing %d\n", majorNumber);
        return majorNumber;
    } 
    else {
        printk(KERN_INFO "Dongle major number successfuly registered with %d\n", majorNumber);
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

    //initialize variables
    key = -1;
    userEnteredKey = -1;

    //initialize the timer
    timer_setup(&myTimer, dgl_time_up, 0);

    printk(KERN_INFO "Dongle device successfully initialized\n");
    return 0;

}

//exit function courtesy DerekMolloy.ie
static void __exit dongle_exit(void) {
    del_timer(&myTimer);
    device_destroy(dglClass, MKDEV(majorNumber, 0));
    class_unregister(dglClass);
    class_destroy(dglClass);
    unregister_chrdev(majorNumber, DEVICE_NAME);
    printk(KERN_INFO "Dongle successfully removed from the system\n");
}

static int dgl_open(struct inode* inp, struct file* fp) {
    printk(KERN_INFO "Dongle successfully opened\n");

    return 0;
}

static int dgl_release(struct inode* inp, struct file* fp) {
    printk(KERN_INFO "Dongle successfully closed\n");
    return 0;
}

//this function takes the key and writes it to the users buffer
static ssize_t dgl_read(struct file* fp, char* buf, size_t lenbuf, loff_t* off) {
    printk(KERN_INFO "Inside dgl_read\n");

    //check if a key exists
    printk(KERN_INFO "dgl_read: key = %d\n", key);
    if(key == -1) {
        printk(KERN_ALERT "dgl_read: we do not have a key to send to user\n");
        return 1;
    }

    //write the key to user's buffer
    error_count = 0;
    sprintf(message, "%d", key);
    sizeOfMessage = strlen(message);
    printk(KERN_INFO "dgl_read: message = %s\n", message);
    printk(KERN_INFO "dgl_read: sizeOfMessage = %d\n", sizeOfMessage);
    error_count = copy_to_user(buf, message, sizeOfMessage);

    //verify all bytes copied to user's buffer
    if(error_count == 0) {            
        printk(KERN_INFO "dgl_read: Sent %d characters to the user\n", sizeOfMessage);
        sizeOfMessage = 0; 
        printk(KERN_INFO "dgl_read: sizeOfMessage reset to %d\n", sizeOfMessage); 
        return 0;
    }
    else {
        printk(KERN_INFO "dgl_read: Sent only %d characters to user instead of %d\n", error_count, sizeOfMessage);
        return -EFAULT;   
    }  
    
    return 0;
}

//write takes a number from user through lenbuf and stores in userEnteredKey
static ssize_t dgl_write(struct file* fp, const char* buf, size_t lenbuf, loff_t* off) {
    printk(KERN_INFO "Inside dgl_write\n");

    //get user input for the key 
    userEnteredKey = lenbuf; 
    printk(KERN_INFO "dgl_write: userEnteredKey: %d\n", userEnteredKey);

    //validate userEnteredKey is correct format
    if((userEnteredKey < MIN_KEY_VALUE) || (userEnteredKey > MAX_KEY_VALUE)) {
        printk(KERN_ALERT "dgl_write: User entered invalid key format\n");
        userEnteredKey = -1;
        return 1;    
    }

    return 0;
}

static long dgl_ioctl(struct file* fp, unsigned int cmd, unsigned long data) {
    printk(KERN_INFO "Inside dgl_ioctl\n");
    switch(cmd) {
        case 667:
            printk(KERN_INFO "dgl_ioctl: case 667: validate user key\n");
            res = dgl_is_correct_key();
            printk(KERN_INFO "dgl_ioctl: case 667: res = %d\n", res);
            if(res == 0) { 
                successFailCode = 0; 
            } 
            else if(res == 1) { 
                successFailCode = 1; 
            } 
            else successFailCode = 2;
            copy_to_user((int32_t*)data, &successFailCode, sizeof(successFailCode));
            return 0;
        case 668:
            printk(KERN_INFO "dgl_ioctl: case 668: about to call dgl_generate\n");
            dgl_generate();
            return 0;
        default:
            printk(KERN_ALERT "dgl_ioctl: Im in default, about to do nothing\n");
            return 1;
    }

    return 0;
}

static void dgl_generate(void) {
    printk(KERN_INFO "dgl_generate: creating key..\n");
    
    get_random_bytes(&key, 4);
    if(key < 0) key = -key;
    if(key < MIN_KEY_VALUE) key += MIN_KEY_VALUE;
    if(key > MAX_KEY_VALUE) key = key / 100;
    
    printk(KERN_INFO "dgl_generate: key %d created\n", key);
    dgl_start_timer();
}

static void dgl_start_timer(void) {
    printk(KERN_INFO "dgl_start_timer: timer\n");

    isTimerUp = 0;
    mod_timer(&myTimer, jiffies + msecs_to_jiffies(MAX_TIME_INTERVAL));

    printk(KERN_INFO "dgl_start_timer: timer successfully started\n");
}

//callback function for timer, called when timer expired
static void dgl_time_up(struct timer_list* data) {
    printk(KERN_INFO "dgl_time_up: the key has expired\n");

    isTimerUp = 1;

    printk(KERN_INFO "dgl_time_up: isTimerUp = %d\n", isTimerUp);
}

static int dgl_is_correct_key(void) {
    //make sure we have both a key and user entered key
    if((key == -1) || (userEnteredKey == -1)) {
        printk(KERN_ALERT "Trying to validate keys with one or both missing\n");
        return 0;
    }

    //output tracers
    printk(KERN_INFO "dgl_is_correct_key: key = %d\n", key);
    printk(KERN_INFO "dgl_is_correct_key: userEnteredKey = %d\n", userEnteredKey);

    //see if we are out of time
    if(isTimerUp) return 2;

    //if userEnteredKey == key return 1
    if(userEnteredKey == key) return 1;
    return 0;
}

module_init(dongle_init);
module_exit(dongle_exit);


