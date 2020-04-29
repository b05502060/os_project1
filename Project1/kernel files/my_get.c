#include <linux/linkage.h>
#include <linux/kernel.h>
#include <linux/ktime.h>
#include <linux/timekeeping.h>

asmlinkage void sys_my_get(struct timespec *t){
	printk("my_get is invoked!\n");
	getnstimeofday(t);
}
