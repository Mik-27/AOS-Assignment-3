/* CSE 536: User-Level Threading Library */
#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "user/user.h"
#include "user/ulthread.h"

/* Standard definitions */
#include <stdbool.h>
#include <stddef.h> 

struct ulthread ulschthread;
struct ulthread uthreads[MAXULTHREADS];
enum ulthread_scheduling_algorithm SCHALGO;

struct ulthread *current_thread = 0;

int nexttid = 1;
int nwthreads = 0;

/* Get thread ID*/
int get_current_tid() {
    return current_thread->tid;
}

/* Thread initialization */
void ulthread_init(int schedalgo) {
    for (int i = 0; i < MAXULTHREADS; i++) {
        uthreads[i].state = FREE;
        uthreads[i].ctime = 0;
        uthreads[i].priority = -1;
        uthreads[i].tid = 0;
    }
    
    ulschthread.state = RUNNABLE;
    ulschthread.ctime = 0;
    ulschthread.priority = -1;
    ulschthread.tid = 0;
    
    SCHALGO = schedalgo;
}

/* Thread creation */
bool ulthread_create(uint64 start, uint64 stack, uint64 args[], int priority) {
    struct ulthread *ut;
    for (ut = uthreads; ut < uthreads+MAXULTHREADS; ut++) {
        if (ut->state == FREE)
            break;
    }
    
    ut->tid = nexttid++;
    
    ut->priority = priority;
    ut->ctime = 0;
    ut->state = RUNNABLE;
    
    memset(&ut->context, 0, sizeof(ut->context));
    ut->context.ra = start;
    ut->context.sp = stack;
    
    // Load the arguments
    ut->context.a0 = args[0];
    ut->context.a1 = args[1];
    ut->context.a2 = args[2];
    ut->context.a3 = args[3];
    ut->context.a4 = args[4];
    ut->context.a5 = args[5];
    
    /* Please add thread-id instead of '0' here. */
    printf("[*] ultcreate(tid: %d, ra: %p, sp: %p)\n", ut->tid, start, stack);
    nwthreads += 1;
    return false;
}

int get_next_tid() {
    int index = -1;
    int curr_tid = -1;
    if (SCHALGO == FCFS) {
        for (int i = 0; i < MAXULTHREADS; i++) {
            if (uthreads[i].state == RUNNABLE){
                if (current_thread != 0 && uthreads[i].tid == get_current_tid()) {
                    curr_tid = i;
                } else if (index == -1) {
                    index = i;
                } else if (uthreads[i].ctime < uthreads[index].ctime)
                    index = i;
            }
        }
    } else if (SCHALGO == ROUNDROBIN) {
        for (int i = 0; i < MAXULTHREADS; i++) {
            int id = (get_current_tid() + i) % MAXULTHREADS;
            if (uthreads[id].state == RUNNABLE){
                if (current_thread != 0 && uthreads[id].tid == get_current_tid()) {
                    curr_tid = id;
                } 
                else if (index == -1) {
                    index = id;
                } 
                else if (uthreads[id].ctime < uthreads[index].ctime)
                    index = id;
            }
        }
    } else if (SCHALGO == PRIORITY) {
        for (int i = 0; i < MAXULTHREADS; i++) {
            if (uthreads[i].state == RUNNABLE){    
                if (current_thread != 0 && uthreads[i].tid == get_current_tid()) {
                    curr_tid = i;
                } else if (index==-1){
                    index = i;
                } else if (uthreads[i].priority > uthreads[index].priority){
                    index = i;
                }
            }
        }
    }
    if (index == -1)
        return curr_tid;
    
    return index;
}

/* Thread scheduler */
void ulthread_schedule(void) {
    while(nwthreads>0) {

        int index = -1;
        int curr_tid = -1;
        if (SCHALGO == FCFS) {
            for (int i = 0; i < MAXULTHREADS; i++) {
                if (uthreads[i].state == RUNNABLE){
                    if (current_thread != 0 && uthreads[i].tid == get_current_tid()) {
                        curr_tid = i;
                    } else if (index == -1) {
                        index = i;
                    } else if (uthreads[i].ctime < uthreads[index].ctime)
                        index = i;
                }
            }
        } else if (SCHALGO == ROUNDROBIN) {
            for (int i = 0; i < MAXULTHREADS; i++) {
                int id = (get_current_tid() + i) % MAXULTHREADS;
                if (uthreads[id].state == RUNNABLE){
                    if (current_thread != 0 && uthreads[id].tid == get_current_tid()) {
                        curr_tid = id;
                    } 
                    else if (index == -1) {
                        index = id;
                    } 
                    else if (uthreads[id].ctime < uthreads[index].ctime)
                        index = id;
                }
            }
        } else if (SCHALGO == PRIORITY) {
            for (int i = 0; i < MAXULTHREADS; i++) {
                if (uthreads[i].state == RUNNABLE){    
                    if (current_thread != 0 && uthreads[i].tid == get_current_tid()) {
                        curr_tid = i;
                    } else if (index==-1){
                        index = i;
                    } else if (uthreads[i].priority > uthreads[index].priority){
                        index = i;
                    }
                }
            }
        }
        if (index == -1)
            index = curr_tid;
        
        ulschthread.state = RUNNABLE;
        current_thread = &uthreads[index];
        current_thread->state = RUNNABLE;
        
        /* Add this statement to denote which thread-id is being scheduled next */
        printf("[*] ultschedule (next tid: %d)\n", current_thread->tid);
        
        // Switch betwee thread contexts
        ulthread_context_switch(&ulschthread.context, &current_thread->context);
    }
}

/* Yield CPU time to some other thread. */
void ulthread_yield(void) {

    /* Please add thread-id instead of '0' here. */
    printf("[*] ultyield(tid: %d)\n", current_thread->tid);
    
    if (SCHALGO != FCFS)
    	current_thread->ctime = ctime();
    
    current_thread->state = RUNNABLE;
    ulschthread.state = RUNNABLE;
    ulthread_context_switch(&current_thread->context, &ulschthread.context); 
}

/* Destroy thread */
void ulthread_destroy(void) {
    current_thread->state = FREE;
    ulschthread.state = RUNNABLE;
    
    printf("[*] ultdestroy(tid: %d)\n", current_thread->tid);
    nwthreads --;
    ulthread_context_switch(&current_thread->context, &ulschthread.context); 
}