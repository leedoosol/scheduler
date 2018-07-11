#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <linux/interrupt.h>
#include <linux/percpu.h>

//#ifdef PER_CPU_HRTIMER
//DEFINE_PER_CPU(int, cnt_per_cpu);
//#endif

MODULE_LICENSE("GPL");
unsigned long timer_interval_ns = 1e7;
static struct hrtimer hr_timer;
static struct workqueue_struct *kworkqueue;
static void blk_mq_iosched_work_fn(struct work_struct *work);
static struct work_struct *mq_iosched_dispatch_works;

DECLARE_PER_CPU(int,cnt_per_cpu);
//DECLARE_DELAYED_WORK(my_work, blk_mq_iosched_work_fn);//1. create work_struct

//process context
static void blk_mq_iosched_work_fn(struct work_struct *work) {

	int cpu_id = get_cpu();
	//trace_printk("in workqueue\n");
	put_cpu();
}
//interrupt context
enum hrtimer_restart timer_callback( struct hrtimer *timer_for_restart) {
	int cpu_id = 0;
	//trace_printk("timer_callback()\n");
	ktime_t currtime, interval;
	currtime = ktime_get();
	interval = ktime_set(0, timer_interval_ns);
	hrtimer_forward(timer_for_restart, currtime, interval);


	for(cpu_id=0; cpu_id<nr_cpu_ids; cpu_id++) {
		queue_work_on(
			cpu_id,
			kworkqueue,
			&mq_iosched_dispatch_works[cpu_id]
		);
	}	
//	queue_delayed_work(kworkqueue, &my_work,0);//3. add work to workqueue  
	//trace_printk("nr_cpu_ids: %d\n",nr_cpu_ids);
	

	return HRTIMER_RESTART;
}

static int __init timer_init(void) {
	int i=0;
	ktime_t ktime;
	ktime = ktime_set(0,timer_interval_ns);
	hrtimer_init(&hr_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	hr_timer.function = &timer_callback;
	hrtimer_start(&hr_timer, ktime, HRTIMER_MODE_REL);
	//2. alloc(create) workqueue   =create_workqueue()
	kworkqueue = alloc_workqueue("kblkschedd", WQ_MEM_RECLAIM,0);
	
	mq_iosched_dispatch_works = kmalloc(
		sizeof(struct work_struct) * nr_cpu_ids,
		GFP_KERNEL
	);
	memset(
		mq_iosched_dispatch_works,
		0,
		sizeof(struct work_struct) * nr_cpu_ids
	);
	for(i=0; i < nr_cpu_ids; i++) {
		INIT_WORK(&mq_iosched_dispatch_works[i], blk_mq_iosched_work_fn);	
	}
	
	int *y;
	int cpu;
	cpu = get_cpu();
	y = per_cpu_ptr(&cnt_per_cpu,cpu);
	*y = 1;
	printk("*y = %d\n", *y);	
	
	return 0;
}
	
static void __exit timer_exit(void) {
	int ret;
	ret = hrtimer_cancel(&hr_timer);
	destroy_workqueue(kworkqueue);
	if(ret) printk("The timer was still in use..\n");
	printk("HR Timer module uninstalling \n");
}

module_init(timer_init);
module_exit(timer_exit);
//EXPORT_PER_CPU_SYMBOL_GPL(cnt_per_cpu);
