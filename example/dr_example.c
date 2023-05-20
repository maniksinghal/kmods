/**
 * @file  drv_example.c
 *
*/
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>

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
static int __init hello_world_init(void)
{
    /** pr_info - aka printk(KERN_INFO ...) */
    pr_info("hello_world: Welcome to linux kernel\n");
    return 0;
}

/**
 * @brief exit function. Called during rmmod
*/
static void __exit hello_world_exit(void)
{
    pr_info("hello_world Kernel module removed successfuly\n");
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
    	pr_info("hello_world callback invoked for my_value = %d\n", my_value);
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


module_init(hello_world_init);   // called during insmod
module_exit(hello_world_exit);   // called during rmmod

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Manik Singhal");
MODULE_DESCRIPTION("Hello world example driver");
MODULE_VERSION("1:1.0");

