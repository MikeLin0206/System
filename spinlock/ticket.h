#define _GNU_SOURCE
#include <stdlib.h>
#include <stdatomic.h>
#include <sched.h>
#include <unistd.h>
#include <stdio.h>


#define contextNum 4
#define maxCores 128

__thread int cpuID;
__thread int tspOrder;

typedef struct globalLock{
    atomic_int wait;
}globalLock_t;

typedef struct ticketLock{
    union{
        atomic_llong grant_ticket;
        struct{
            int grant;
            int ticket;
        };
    };    
}ticketLock_t;

ticketLock_t arrayLock[64];

void lockInit(globalLock_t *lock){
    lock -> wait = 0;
    for(int i = 0;i < 64;i++){
        arrayLock[i].grant = 0;
        arrayLock[i].ticket = 1;
    }
}

void lockAcquire(globalLock_t *lock){
    if(atomic_fetch_add(&lock -> wait, 1) == 0)
        return;   

    int localTicket = atomic_fetch_add(&arrayLock[tspOrder].ticket, 1); //memory_order_relaxed

    while(localTicket != atomic_load(&arrayLock[tspOrder].grant))  //memory_order_relaxed
        asm("pause");
    return;
}

void lockRelease(globalLock_t *lock){
    int one = 1;
    if(atomic_compare_exchange_strong(&lock -> wait, &one, 0))
        return;

    atomic_fetch_sub(&lock -> wait, 1); //memory_order_acquire

    int next = (tspOrder + 1) % 64;
    while(1){
        if((atomic_load(&arrayLock[next].grant) - atomic_load(&arrayLock[next].ticket)) <= -2){
            //memory_order_acquire
            atomic_fetch_add(&arrayLock[next].grant, 1);
            return;
        }
        next = (next + 1) % 64;
    }
}
