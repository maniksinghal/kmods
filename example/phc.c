/**
 * @file  drv_example.c
 *
*/
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/cdev.h>


/**
 * Quick user commands
 * Listing modules loaded in the kernel: lsmod
 * Listing devices created by drivers: cat /proc/devices
 * Checking parameters offered by a module: /sys/module/<module-name>/parameters/<param-name>
 */

int create_device_files(void);

#define NUM_PHC_DEVICES  2
static bool device_registered = false;
static dev_t device = 0;
static struct class *dev_class = NULL;
static bool device_files_created[NUM_PHC_DEVICES] = {false};
static struct cdev phc_cdev;
static bool cdev_added = false;


/** Global variables which will/can serve as module parameters.
 *  They are set/get by user by writing to their sysfs paths
 */
int my_value = 0;  // default value
char *name = "manik singhal";

/** Declare required global variables as module parameters 
 *
 *  User can access them by echo/cat to: /sys/module/dr_example/parameters/<var-name>
 *    Shall require sudo access
 *
 *  Possible types for variables: 
 *    bool/invbool charp int/long/short/uint/ulong/ushort
 */
module_param(name, charp, S_IRUSR|S_IWUSR); //read/write permissions for user

/**
 * @brief Init function. Called during insmod
*/
static int __init phc_driver_init(void)
{
    int rc = 0;

    /** pr_info - aka printk(KERN_INFO ...) */
    pr_info("phc_driver: Welcome to linux kernel\n");

    /** Register 2 PHC clock devices with the kernel 
     *
     * About Major and Minor versions:
     * - Major version describes a driver (some drivers
           can share a major version)
     * - Minor version identifies a device
     * - Two devices cannot have same major and minor 
         versions.

     * Ask kernel to allocate a free major version, so that
     *   we can use minor versions 0 and 1.
    */
    rc = alloc_chrdev_region(&device, 0, NUM_PHC_DEVICES, "phc");
    if (rc != 0) {
        pr_err("phc_driver: Kernel unable to allocate major/minor versions. rc:%d\n", rc);
        return -1;
    }

    /** Successfully registered. Now "phc" driver shall be visible in
     *  cat /proc/devices
     */
    device_registered = true;
    pr_info("phc_driver: kernel allocated major:%d, minor:%d\n",
            MAJOR(device), MINOR(device));

    /** Create devices (/dev/phc_device */
    create_device_files();

    return 0;
}

/**
 * @brief exit function. Called during rmmod
*/
static void __exit phc_driver_exit(void)
{
    int idx = 0;
    dev_t tmp_dev = 0;

    if (device_registered) {
        for (idx = 0; idx < NUM_PHC_DEVICES; idx++) {
            if (device_files_created[idx]) {
                tmp_dev = MKDEV(MAJOR(device), idx);
                device_destroy(dev_class, tmp_dev);
                device_files_created[idx] = false;
            }
        }

        if (dev_class) {
            class_destroy(dev_class);
            dev_class = NULL;
        }

        if (cdev_added) {
            cdev_del(&phc_cdev);
            cdev_added = false;
        }
        (void)unregister_chrdev_region(device, NUM_PHC_DEVICES);
        device_registered = false;
    }
    pr_info("phc_driver: Kernel module removed successfuly\n");
}

/**
 * Callback function for whenever user sets/gets my_value
 * Registered below
 */
int notify_my_value_set(const char *val, const struct kernel_param *kp)
{
    /** Use standard helper function for setting the value
     * As this callback is registered for the param my_value, 
     * so the my_value will end up getting set by this call
     */
    int res = param_set_int(val, kp);  
    if (res == 0) { 
    	pr_info("phc_driver: callback invoked for my_value = %d\n", my_value);
    }
    return 0;
}

/**
 * setter/getter callbacks for my_value param 
 */
const struct kernel_param_ops my_value_param_ops = 
{
    .set = &notify_my_value_set,   // Using our defined setter cb above
    .get = &param_get_int,         // Using standard getter
};

/** Declare my_value as a module param with callbacks */
module_param_cb(my_value, &my_value_param_ops, &my_value, S_IRUGO|S_IWUSR);


module_init(phc_driver_init);   // called during insmod
module_exit(phc_driver_exit);   // called during rmmod

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Manik Singhal");
MODULE_DESCRIPTION("Hello world example driver");
MODULE_VERSION("1:1.0");

/**
 * File operation callbacks for our device
 */

/** Device open callback. If not provided, then device-open call from 
 * application is always successful */
static int phc_open(struct inode *inode, struct file *file);

/** Device close callback */
static int phc_release(struct inode *inode, struct file *file);

/** Device read */
static ssize_t phc_read(struct file *file, char __user *buf, size_t len, loff_t *off);

/** Device write */
static ssize_t phc_write(struct file *file, const char *buf, size_t len, loff_t *off);

static struct file_operations fops = 
{
    .owner  = THIS_MODULE,
    .read   = phc_read,
    .write  = phc_write,
    .open   = phc_open,
    .release = phc_release,
};

/**
 * Device file open callback
*/
static int
phc_open(struct inode *inode, struct file *file)
{
    pr_info("phc_driver: File open call!!\n");
    return 0;
}

/**
 * Device file close callback
*/
static int
phc_release(struct inode *inode, struct file *file)
{
    pr_info("phc_driver: File release called !!\n");
    return 0;
}

/**
 * Device read callback
*/
static ssize_t
phc_read(struct file *file, char __user *buf, size_t len, loff_t *off)
{
    pr_info("phc_driver: File read called!!\n");
    return len;
}

/**
 * Device write callback
*/
static ssize_t
phc_write(struct file *file, const char __user *buf, size_t len, loff_t *off)
{
    pr_info("phc_driver: File write called!!\n");
    return len;
}

int
create_device_files()
{
    struct device *dev = NULL;
    int idx = 0;
    char device_name[] = "phcX";
    int dev_num_position = 0;
    dev_t tmp_dev = 0;
    int res = 0;

    /** Create a device class, used below to create the device file */
    dev_class = class_create(THIS_MODULE, "phc_class");
    if (IS_ERR(dev_class)) {
        pr_err("phc_driver: Cannot create device class.\n");
        return -1;
    }
    pr_info("phc_driver: Device class created successfully\n");

    /** Create the cdev structure for attaching file 
     * operations callbacks to the device */
    cdev_init(&phc_cdev, &fops);
    res = cdev_add(&phc_cdev, device, NUM_PHC_DEVICES);
    if (res < 0) {
        pr_err("phc_driver: Could not cdev_add the file operations to device");
        class_destroy(dev_class);
        dev_class = NULL;
        return -1;
    }
    cdev_added = true;

    /* Now create the device */
    dev_num_position = sizeof(device_name) -2;
    for (idx = 0; idx < NUM_PHC_DEVICES; idx++) {
        device_name[dev_num_position] = '0' + idx;
        tmp_dev = MKDEV(MAJOR(device), idx);
        dev = device_create(dev_class, NULL, tmp_dev, NULL, device_name);
        if (IS_ERR(dev)) {
            pr_err("phc_driver: Cannot create the device\n");
            class_destroy(dev_class);
            dev_class = NULL;
            return -1;
        }
        pr_info("phc_driver: Created device %s\n", device_name);
        device_files_created[idx] = true;
    }
    pr_info("phc_driver: Device created successfully\n");
    return 0;
}


    

