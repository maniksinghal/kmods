#include <linux/module.h>
#include <linux/ptp_clock_kernel.h>
#include <linux/ktime.h>

#define cl_debug_info(args...) \
    if (cl_debug) pr_info(args)


static struct ptp_clock *myclock;

/* clock parameters */
ktime_t base_ts;
s64 ns_offset = 0;
int ppb_drift = 0;

/* user provided value through sysfs */
long user_seconds = 0;

int cl_debug = 0;  // flag to enable/disable debug

static void
reset_base_ts(void)
{
	base_ts = ktime_get();
}

static int myclock_adjfine(struct ptp_clock_info *info, long scaled_ppm)
{
    /* Adjust clock frequency */
	int sign = 1;  // positive by default
	s64 ppb = 0;

	if (scaled_ppm < 0) {
		scaled_ppm = -scaled_ppm;
		sign = -1;
	}

	ppb = (scaled_ppm >> 16) * 1000;
	ppb_drift = ppb * sign;

	reset_base_ts();

    return 0;
}

static int myclock_adjtime(struct ptp_clock_info *info, s64 delta)
{
    /* Adjust clock time */
	ns_offset += delta;
	cl_debug_info("Clock adjust by %lld, new-offset:%lld\n", delta, ns_offset);

    return 0;
}

static int myclock_gettime64(struct ptp_clock_info *info, struct timespec64 *ts)
{
    /* Get current clock time */
	s64 ns;
    ktime_t kt = ktime_get();

	pr_info("myclock_gettime64 invoked\n");

	ns = kt - base_ts;
	ns = ns + ((ns * ppb_drift)/1000000000);
	ns += ns_offset;
	ns += base_ts;

	*ts = ns_to_timespec64(ns);

    return 0;
}


static int myclock_settime64_internal(const struct timespec64 *ts)
{
    /* Set clock time */
	s64 ns = timespec64_to_ns(ts);
	ns_offset = ns - ktime_get();
	pr_info("myclock_settime64. ns_offset:%lld\n", ns_offset);

    return 0;
}

static int myclock_settime64(struct ptp_clock_info *info, const struct timespec64 *ts)
{
	return myclock_settime64_internal(ts);
}

static int myclock_enable(struct ptp_clock_info *info, struct ptp_clock_request *rq, int on)
{
    /* Enable or disable the clock */
    /* Implement your own logic here */

    return 0;
}

static int myclock_adjfreq(struct ptp_clock_info *info, s32 delta)
{
    /* Adjust clock frequency */
    /* Implement your own logic here */

    return 0;
}

static struct ptp_clock_info myclock_info = {
    .owner      = THIS_MODULE,
    .name       = "myclock",
    .max_adj    = 1000000000,   /* maximum frequency adjustment in parts per billion (ppt) */
    .adjfine    = myclock_adjfine,
    .adjtime    = myclock_adjtime,
    
    .gettime64    = myclock_gettime64,
    .settime64    = myclock_settime64,
    .enable     = myclock_enable,
    .adjfreq    = myclock_adjfreq,
};

static int __init myclock_init(void)
{
    int err;

    myclock = ptp_clock_register(&myclock_info, NULL);
    if (IS_ERR(myclock)) {
        err = PTR_ERR(myclock);
        pr_err("Failed to register PTP clock: %d\n", err);
        return err;
    }

	reset_base_ts();
    pr_info("PTP clock registered successfully\n");
    return 0;
}

static void __exit myclock_exit(void)
{
    ptp_clock_unregister(myclock);
    pr_info("PTP clock unregistered\n");
}

module_init(myclock_init);
module_exit(myclock_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Manik Singhal");
MODULE_DESCRIPTION("PTP clock module");

static int 
set_seconds_from_user(const char *val, const struct kernel_param *kp)
{
	struct timespec64 ts;
    /** Use standard helper function for setting the value
     * As this callback is registered for the param user_seconds, 
     * so the user_seconds will end up getting set by this call
     */
    int res = param_set_long(val, kp);  
    if (res == 0) { 
		ts.tv_sec = user_seconds;
		ts.tv_nsec = 0;
		myclock_settime64_internal(&ts);
    	pr_info("User specified seconds: %ld\n", user_seconds);

    }
    return 0;
}

/**
 * setter/getter callbacks for user_seconds param 
 */
const struct kernel_param_ops user_seconds_param_ops = 
{
    .set = &set_seconds_from_user,   // Using our defined setter cb above
    .get = &param_get_int,         // Using standard getter
};

/** Declare my_value as a module param with callbacks */
module_param_cb(user_seconds, &user_seconds_param_ops, &user_seconds, S_IRUGO|S_IWUSR);
module_param(cl_debug, int, S_IRUSR|S_IWUSR); //read/write permissions for user
