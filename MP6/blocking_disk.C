/*
     File        : blocking_disk.c

     Author      :
     Modified    :

     Description :

*/

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

    /* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "assert.H"
#include "utils.H"
#include "console.H"
#include "scheduler.H"
#include "blocking_disk.H"

extern Scheduler* SYSTEM_SCHEDULER;

/*--------------------------------------------------------------------------*/
/* CONSTRUCTOR */
/*--------------------------------------------------------------------------*/

BlockingDisk::BlockingDisk(DISK_ID _disk_id, unsigned int _size)
  : SimpleDisk(_disk_id, _size) {
    SYSTEM_SCHEDULER->registerDisk(this);
    blockedStart = 0;
    blockedEnd = 0;
    blockedQueueEmpty = true;
    blockedCapacity = 4;
    blockedThreads = new Thread*[blockedCapacity];
}

/*--------------------------------------------------------------------------*/
/* SIMPLE_DISK FUNCTIONS */
/*--------------------------------------------------------------------------*/

void BlockingDisk::read(unsigned long _block_no, unsigned char * _buf) {

    // put current thread on waiting queue
    addBlocked(Thread::CurrentThread());

    // wait for requests that were issued before you
    while (getNextBlocked() != Thread::CurrentThread())
    {
        SYSTEM_SCHEDULER->yield();
    }

    issue_operation(READ, _block_no);
    SYSTEM_SCHEDULER->yield();

    /* read data from port */
    int i;
    unsigned short tmpw;
    for (i = 0; i < 256; i++) {
      tmpw = inportw(0x1F0);
      _buf[i*2]   = (unsigned char)tmpw;
      _buf[i*2+1] = (unsigned char)(tmpw >> 8);
    }
}


void BlockingDisk::write(unsigned long _block_no, unsigned char * _buf) {

    // put current thread on waiting queue
    addBlocked(Thread::CurrentThread());

    // wait for requests that were issued before you
    while (getNextBlocked() != Thread::CurrentThread())
    {
        SYSTEM_SCHEDULER->yield();
    }

    issue_operation(WRITE, _block_no);
    SYSTEM_SCHEDULER->yield();

    /* write data to port */
    int i;
    unsigned short tmpw;
    for (i = 0; i < 256; i++) {
      tmpw = _buf[2*i] | (_buf[2*i+1] << 8);
      outportw(0x1F0, tmpw);
    }
}

void BlockingDisk::checkAndResume()
{
    if (!blockedQueueEmpty && is_ready())
    {
        Thread* nowReady = popNextBlocked();
        SYSTEM_SCHEDULER->resume(nowReady);
        if (!blockedQueueEmpty)
        {
            SYSTEM_SCHEDULER->resume(getNextBlocked());
        }
    }
}

Thread* BlockingDisk::getNextBlocked()
{
    return blockedThreads[blockedStart];
}

Thread* BlockingDisk::popNextBlocked()
{
    Thread* head = blockedThreads[blockedStart];
    blockedStart = (blockedStart + 1) % blockedCapacity;
    if (blockedStart == blockedEnd)
    {
        blockedQueueEmpty = true;
    }
    return head;
}

void BlockingDisk::addBlocked(Thread* t)
{
    if (blockedStart == blockedEnd && !blockedQueueEmpty)
    {
        Thread** newQueue = new Thread*[blockedCapacity * 2];
        for (unsigned long i = 0; i < blockedCapacity; ++i)
        {
            newQueue[i] = blockedThreads[(blockedStart + i) % blockedCapacity];
        }
        blockedStart = 0;
        blockedEnd = blockedCapacity;
        blockedCapacity *= 2;
        delete[] blockedThreads;
        blockedThreads = newQueue;
    }
    blockedThreads[blockedEnd] = t;
    blockedEnd = (blockedEnd + 1) % blockedCapacity;
    blockedQueueEmpty = false;
}
