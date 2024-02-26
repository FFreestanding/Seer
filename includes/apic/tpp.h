#ifndef _TPP_H_
#define _TPP_H_

/*
    Task and Processor Priorities
    The local APIC also defines a task priority and a processor priority that determine the order in which interrupts
    are handled. The task-priority class is the value of bits 7:4 of the task-priority register (TPR), which can be
    written by software (TPR is a read/write register); see Figure 10-18.
*/
// if (interrupt vector number >= 16*TC+TSC) , it will generate interrupt

// 0-3 Task-Priority Sub-Class
#define TASK_PRIORITY_SUB_CLASS(x) ((x)&0b1111)
// 4-7 Task-Priority Class
#define TASK_PRIORITY_CLASS(x) ((x&0b1111)<<4)
// 8-31 Reserved
#define TASK_PRIORITY_RESERVED(x) ((x&0xffffff)<<8)

#define TPR_DEFAULT(class, sub_class) (TASK_PRIORITY_CLASS(class)|TASK_PRIORITY_SUB_CLASS(sub_class))

#endif // _TPP_H_