#include <linux/linkage.h>
#include <linux/kernel.h>
#include <linux/ktime.h>
#include <linux/timekeeping.h>

asmlinkage void sys_my_print(int pid,struct timespec *s,struct timespec *e){
	printk("[Project1] %d %lu.%lu %lu.%lu\n",pid,s->tv_sec,s->tv_nsec,e->tv_sec,e->tv_nsec);
}
