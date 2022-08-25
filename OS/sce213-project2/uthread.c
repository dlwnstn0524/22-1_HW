#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/time.h>
#include <ucontext.h>
#include<string.h>
#include "uthread.h"
#include "list_head.h"
#include "types.h"

/* You can include other header file, if you want. */

/*******************************************************************
 * struct tcb
 *
 * DESCRIPTION
 *    tcb is a thread control block.
 *    This structure contains some information to maintain thread.
 *
 ******************************************************************/
struct tcb {
    struct list_head list;
    ucontext_t *context;
    enum uthread_state state;
    int tid;
    int lifetime; // This for preemptive scheduling
    int priority; // This for priority scheduling
};

/***************************************************************************************
 * LIST_HEAD(tcbs);
 *
 * DESCRIPTION
 *    Initialize list head of thread control block.
 *
 **************************************************************************************/
LIST_HEAD(tcbs);

int n_tcbs = 0;

struct ucontext_t *t_context;

struct tcb *current;

enum uthread_sched_policy now_policy;

/***************************************************************************************
 * next_tcb()
 *
 * DESCRIPTION
 *
 *    Select a tcb with current scheduling policy
 *
 **************************************************************************************/
void next_tcb() {
    struct tcb *next = current;
    struct tcb *prev;
    if(now_policy == FIFO){
       next = fifo_scheduling(next);   
    }
    else if (now_policy == SJF){
        next = sjf_scheduling(next);
    }
    else if(now_policy == PRIO){
        next = prio_scheduling(next);
    }
    else if(now_policy == RR){
        next = rr_scheduling(next);
    }
    
    prev = current;
    current = next;
    if(now_policy == 1 || now_policy == 2){
        // fprintf(stderr, "current->tid : %d || current->lifetime : %d\n", current->tid, current->lifetime);
        if(prev->tid != -1 || current->tid != -1){ 
            //if(prev->tid != current->tid){
                fprintf(stderr, "SWAP %d -> %d\n", prev->tid, current->tid); 
                swapcontext(prev->context, current->context);
            //}
        }
    } 
    else{
        if(prev->tid != current->tid){
            fprintf(stderr, "SWAP %d -> %d\n", prev->tid, current->tid); 
            swapcontext(prev->context, current->context);
        }
    }
}

/***************************************************************************************
 * struct tcb *fifo_scheduling(struct tcb *next)
 *
 * DESCRIPTION
 *
 *    This function returns a tcb pointer using First-In-First-Out policy
 *
 **************************************************************************************/
struct tcb *fifo_scheduling(struct tcb *next) { // FIFO 알고리즘 자체를 구현하면 됨. 다음 쓰레드 판단 기준은 오로지 list_head 순으로
    /* TODO: You have to implement this function. */
    list_for_each_entry(next, &tcbs, list){
        //fprintf(stderr, "next->tid : %d || next->state : %d\n", next->tid, next->state);
        if(next->state == READY && next->tid != -1){ // 초기 쓰레드 제외하고 READY인 쓰레드 모두 TERMINATED 시키기
            next->state = TERMINATED;
            break;
        }
    }
    if(&next->list == &tcbs){  
        next = list_next_entry(next, list);
    }
    //fprintf(stderr, "next->tid : %d || next->state : %d\n", next->tid, next->state);
    return next;

}

/***************************************************************************************
 * struct tcb *rr_scheduling(struct tcb *next)
 *
 * DESCRIPTION
 *
 *    This function returns a tcb pointer using round robin policy
 *
 **************************************************************************************/
struct tcb *rr_scheduling(struct tcb *next) {
    /* TODO: You have to implement this function. */
    // fprintf(stderr, "next->tid : %d || next->state : %d\n", next->tid, next->state);
    //next = list_next_entry(next, lislt);
    next = list_next_entry(next, list);
    if(&next->list == &tcbs){  
        next = list_next_entry(next, list);
    }
    while(next->state == TERMINATED || next->lifetime == 0){
        next = list_next_entry(next, list);
        if(&next->list == &tcbs){  
        next = list_next_entry(next, list);
        }
    }
    if(next->lifetime == 1){
        next->state = TERMINATED;
    }
    next->lifetime--;  
    return next;
}

/***************************************************************************************
 * struct tcb *prio_scheduling(struct tcb *next)
 *
 * DESCRIPTION
 *
 *    This function returns a tcb pointer using priority policy
 *
 **************************************************************************************/
struct tcb *prio_scheduling(struct tcb *next) {
    /* TODO: You have to implement this function. */
    int max = MAIN_THREAD_PRIORITY - 1;
    struct tcb* temp = (struct tcb*)malloc(sizeof(struct tcb));
    list_for_each_entry(temp, &tcbs, list){
        if(temp->state == READY && temp->priority > max){
            max = temp->priority;
            next = temp;
        }
    }
    if(next->lifetime == 1){
        next->state = TERMINATED;
    }
    else{
        next->lifetime--;
    }
    return next;
}

/***************************************************************************************
 * struct tcb *sjf_scheduling(struct tcb *next)
 *
 * DESCRIPTION
 *
 *    This function returns a tcb pointer using shortest job first policy
 *
 **************************************************************************************/
struct tcb *sjf_scheduling(struct tcb *next) { // FIFO 알고리즘 자체를 구현하면 됨. 다음 쓰레드 판단 기준은 list_head 순으로 lifetime 조회
    /* TODO: You have to implement this function. */
    int min = MAIN_THREAD_LIFETIME + 1;
    struct tcb* temp = (struct tcb*)malloc(sizeof(struct tcb));
    struct tcb* tmp = (struct tcb*)malloc(sizeof(struct tcb));
    tmp = next;
    list_for_each_entry(temp, &tcbs, list){ //5 6 7 -> 7 5 6 
        //fprintf(stderr,"next->tid : %d || next->state:  %d\n", next->tid, next->state);
        if(temp->state == READY && temp->lifetime < min){
            min = temp->lifetime;
            next = temp;
        }
    }
    if(next->tid == tmp->tid){
        next = list_first_entry(&tcbs,struct tcb,list);
    }
    next->state = TERMINATED;
    // if(&next->list == &tcbs){  
    //     next = list_next_entry(next, list);
    // }
    // fprintf(stderr,"next->tid : %d || next->state:  %d\n", next->tid, next->state);
    return next;

}

/***************************************************************************************
 * uthread_init(enum uthread_sched_policy policy)
 *
 * DESCRIPTION

 *    Initialize main tread control block, and do something other to schedule tcbs
 *
 **************************************************************************************/
void uthread_init(enum uthread_sched_policy policy) {
    /* TODO: You have to implement this function. */
    struct tcb *main_thread = (struct tcb *)malloc(sizeof(struct tcb));
    main_thread->context = (ucontext_t*)malloc(sizeof(ucontext_t)+MAX_STACK_SIZE);
    main_thread->state = READY;
    main_thread->tid = MAIN_THREAD_TID;
    main_thread->lifetime = MAIN_THREAD_LIFETIME;
    main_thread->priority = MAIN_THREAD_PRIORITY;
    n_tcbs++;
    now_policy = policy;
    current = main_thread;
    getcontext(main_thread->context);
    t_context = current->context;
    list_add_tail(&main_thread->list, &tcbs);
    /* DO NOT MODIFY THESE TWO LINES */
    __create_run_timer();
    __initialize_exit_context();
}

/***************************************************************************************
 * uthread_create(void* stub(void *), void* params)
 *
 * DESCRIPTION
 *
 *    Create user level thread. This function returns a tid.
 *
 **************************************************************************************/
int uthread_create(void* stub(void *), void* args) {
    /* TODO: You have to implement this function. */
    struct tcb* entry = (struct tcb*)malloc(sizeof(struct tcb));
    entry->context = (ucontext_t*)malloc(sizeof(ucontext_t)+MAX_STACK_SIZE);
    entry->state = READY;
    int* params = args;
    entry->tid = params[0];
    entry->lifetime = params[1];
    entry->priority = params[2];
    n_tcbs++;
    getcontext(entry->context);
    entry->context->uc_link = t_context;
    entry->context->uc_stack.ss_sp = (void*)malloc(MAX_STACK_SIZE);
    entry->context->uc_stack.ss_flags = 0;
    entry->context->uc_stack.ss_size = MAX_STACK_SIZE;
    if(now_policy == 1 || now_policy == 2){
        sigemptyset(&entry->context->uc_sigmask);
    }
    makecontext(entry->context, (void *)stub, 0);
    list_add_tail(&entry->list, &tcbs);
    return entry->tid;
}

/***************************************************************************************
 * uthread_join(int tid)
 *
 * DESCRIPTION
 *
 *    Wait until thread context block is terminated.
 *
 **************************************************************************************/
void uthread_join(int tid) { // 입력된 tid와 동일한 쓰레드를 찾기 -> state가 TERMINATED가 될때까지 무한루프
    
    /* TODO: You have to implement this function. */
    struct tcb *a;
	list_for_each_entry(a, &tcbs, list){
		if(a->tid == tid){
            while(1){
                if(a->state == TERMINATED){
                    break;
                }
            }
            fprintf(stderr, "JOIN %d\n", a->tid);
        }
	}
}

/***************************************************************************************
 * __exit()
 *
 * DESCRIPTION
 *
 *    When your thread is terminated, the thread have to modify its state in tcb block.
 *
 **************************************************************************************/
void __exit() {
    /* TODO: You have to implement this function. */

}

/***************************************************************************************
 * __initialize_exit_context()
 *
 * DESCRIPTION
 *
 *    This function initializes exit context that your thread context will be linked.
 *
 **************************************************************************************/
void __initialize_exit_context() {
    /* TODO: You have to implement this function. */

}

/***************************************************************************************
 *
 * DO NOT MODIFY UNDER THIS LINE!!!
 *
 **************************************************************************************/

static struct itimerval time_quantum;
static struct sigaction ticker;

void __scheduler() {
    if(n_tcbs > 1)
        next_tcb();
}

void __create_run_timer() {
    time_quantum.it_interval.tv_sec = 0;
    time_quantum.it_interval.tv_usec = SCHEDULER_FREQ;
    time_quantum.it_value = time_quantum.it_interval;

    ticker.sa_handler = __scheduler;
    sigemptyset(&ticker.sa_mask);
    sigaction(SIGALRM, &ticker, NULL);
    ticker.sa_flags = 0;

    setitimer(ITIMER_REAL, &time_quantum, (struct itimerval*) NULL);
}

void __free_all_tcbs() {
    struct tcb *temp;
    list_for_each_entry(temp, &tcbs, list) {
        if (temp != NULL && temp->tid != -1) {
            list_del(&temp->list);
            free(temp->context);
            free(temp);
            n_tcbs--;
            temp = list_first_entry(&tcbs, struct tcb, list);
        }
    }
    temp = NULL;
    free(t_context);
}