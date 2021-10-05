#define _GNU_SOURCE
#include <stdlib.h>
#include <stdatomic.h>
#include <sched.h>
#include <unistd.h>
#include <stdio.h>

#define contextNum 4
#define maxCores 64 //sysconf(_SC_NPROCESSORS_ONLN)

__thread int cpuID;
__thread int tspOrder;

typedef struct qspinlock {
     union {
         atomic_int val;
 
         struct {
             unsigned char  locked;
             unsigned char  pending;
         };
         struct {
             unsigned short locked_pending;
             unsigned char head;
             unsigned char tail;
         };
     };
}spinlock_t;


struct mcs_spinlock {
    struct mcs_spinlock *next;
    int locked; /* 1 if lock acquired */
    unsigned long long tsp;
    int cpu;
};



struct qnode{
    struct mcs_spinlock mcs;
};

void lockInit(spinlock_t *lock){
    lock -> val = 0;
}
struct qnode waitQueue[maxCores][contextNum];

struct mcs_spinlock blankNode;

#define _Q_LOCKED_BITS 8
#define PENDING_STATE 1U << 8
#define LOCKED_MASK (1U << 8) - 1
void slowpath(spinlock_t *lock){
    int val = atomic_load_explicit(&lock -> val, memory_order_relaxed);
    //check when pending = 1, locked = 0
    if(val == PENDING_STATE){
        do{
            val = atomic_load_explicit(&lock -> val, memory_order_relaxed);          
        }while(val == PENDING_STATE);    
    }

    //if pending != 0 , tail != 0
    if(val & ~(LOCKED_MASK))
        goto queue;

    //set pending = 1
    val = atomic_fetch_add_explicit(&lock -> pending, 1, memory_order_acquire);
    //spining until locked = 0
    if(val & LOCKED_MASK){    
        do{
            val = atomic_load_explicit(&lock -> val, memory_order_relaxed);
        }while(val & LOCKED_MASK);        
    }
    //set locked = 1
    atomic_store_explicit(&lock -> locked_pending, 1, memory_order_release);
    return;

queue:;
    struct mcs_spinlock *node = &(waitQueue[cpuID][0].mcs);
    // goto unlock section
    node -> locked = 0;
    //node -> next = NULL;

search:;
    char zero = 0;
    //test it's head or not
    char firstNode = atomic_compare_exchange_strong_explicit(&lock -> head, &zero, cpuID + 1, memory_order_release,
                                                                                              memory_order_relaxed);  
    if(!firstNode){
        char head = atomic_load_explicit(&lock -> head, memory_order_acquire) - 1;
        struct mcs_spinlock prev = waitQueue[head][0].mcs;
        struct mcs_spinlock *prevPointer = &waitQueue[head][0].mcs;
        struct mcs_spinlock *next = prev.next;

        while(node -> tsp < prev.tsp){
            node -> tsp += 64;
        }

        //Searching until to tail or find the node that the tsp is larger and previous node is in the same turn
        while(1){
            if(next == &blankNode)
                goto search;
            if(next == NULL)
                break;
            if(next -> tsp > node -> tsp)
                break;

            prevPointer = next; 
            prev = *next; 
            next = atomic_load_explicit(&next -> next, memory_order_acquire);
        } 
        
        node -> next = next;  
        //Test the previous node is in the queue and next field has been inserted or not
        if(!atomic_compare_exchange_weak_explicit(&prevPointer -> next, &prev.next, node, memory_order_release, memory_order_relaxed)){
            goto search; 
        }

        //Spinning
        while(!atomic_load_explicit(&node -> locked, memory_order_acquire))
            ;
    }
    else{
        if(node -> next == &blankNode){
            atomic_store_explicit(&node -> next, NULL, memory_order_release);
        }
    }

    //locked and pending = 11111111
    #define _Q_LOCKED_PENDING_MASK ((1U << _Q_LOCKED_BITS) - 1 | (((1U << _Q_LOCKED_BITS) - 1) << 8))
    //spining on locked and pending, until locked and pending = 0
    do{
        val = atomic_load_explicit(&lock -> val, memory_order_acquire);
    }while(val & _Q_LOCKED_PENDING_MASK);

    //Get the lock and leave the queue
    //if next == NULL, means it's the only node in the queue, use compare exchange to set val = 1(locked = 1, pending = tail = 0)
    struct mcs_spinlock *np = NULL;
    struct mcs_spinlock *nextNode = atomic_exchange_explicit(&waitQueue[cpuID][0].mcs.next, &blankNode, memory_order_acq_rel);
    if(nextNode == NULL){ 
        atomic_store_explicit(&lock -> val, 1, memory_order_release);
        return;
    }

    //if not, set locked = 1 and next locked of next node = 1
    atomic_store_explicit(&lock -> head, nextNode -> cpu, memory_order_release); 
    atomic_store_explicit(&lock -> locked, 1, memory_order_relaxed);
    atomic_store_explicit(&nextNode -> locked, 1, memory_order_release);
}

void lockAcquire(spinlock_t *lock){
    int zero = 0;
    if(atomic_compare_exchange_strong_explicit(&(lock -> val), &zero, 1, memory_order_acquire, memory_order_relaxed))
	return;
 
    slowpath(lock);
}

void lockRelease(spinlock_t *lock){
    atomic_store_explicit(&lock -> locked, 0, memory_order_release);
}
