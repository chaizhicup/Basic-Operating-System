/*
 File: scheduler.C

 Author:
 Date  :

 */

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "scheduler.H"
#include "thread.H"
#include "console.H"
#include "utils.H"
#include "assert.H"
#include "simple_keyboard.H"
#include "blocking_disk.H"

/*--------------------------------------------------------------------------*/
/* DATA STRUCTURES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* CONSTANTS */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* FORWARDS */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* METHODS FOR CLASS   S c h e d u l e r  */
/*--------------------------------------------------------------------------*/

Scheduler::Scheduler() {
    readyCapacity = 4;
    readyStart = 0;
    readyEnd = 0;
    empty = true;
    terminated = NULL;
    readyQueue = new Thread*[readyCapacity];
    diskSize = 0;
    diskCapacity = 1;
    disks = new BlockingDisk*[diskCapacity];
    Console::puts("Constructed Scheduler.\n");
}

void Scheduler::registerDisk(BlockingDisk* disk)
{
    if (diskSize == diskCapacity)
    {
        diskCapacity *= 2;
        BlockingDisk** newDisks = new BlockingDisk*[diskCapacity];
        for (unsigned long i = 0; i < diskSize; ++i)
        {
            newDisks[i] = disks[i];
        }
        delete[] disks;
        disks = newDisks;
    }
    disks[diskSize++] = disk;
}

void Scheduler::yield() {

    for (unsigned long i = 0; i < diskSize; ++i)
    {
        disks[i]->checkAndResume();
    }

    Thread* nextThread = readyQueue[readyStart++];
    readyStart %= readyCapacity;
    if (readyStart == readyEnd)
    {
        empty = true;
    }
    Thread::dispatch_to(nextThread);
}

void Scheduler::resume(Thread * _thread) {
    if (readyEnd == readyStart && !empty)
    {
        Thread** nextReadyQueue = new Thread*[readyCapacity * 2];
        for (int i = 0; i < readyCapacity; ++i)
        {
            nextReadyQueue[i] = readyQueue[(readyStart + i) % readyCapacity];
            readyEnd = i;
        }
        readyCapacity *= 2;
        delete[] readyQueue;
        readyQueue = nextReadyQueue;
        readyStart = 0;
    }
    readyQueue[readyEnd++] = _thread;
    readyEnd %= readyCapacity;
    empty = false;
}

void Scheduler::add(Thread * _thread) {
    resume(_thread);
}

void Scheduler::terminate(Thread * _thread) {
    // search ready queue for _thread
    int i = readyStart;
    if (readyStart == readyEnd && !empty)
    {
        if (_thread != readyQueue[i])
        {
            i = (i + 1) % readyCapacity;
        }
    }
    for (; i != readyEnd; i = (i + 1) % readyCapacity)
    {
        if (_thread == readyQueue[i])
        {
            break;
        }
    }
    // remove it if you find it
    if (i != readyEnd)
    {
        readyStart = (readyStart + 1) % readyCapacity;
        if (readyStart == readyEnd)
        {
            empty = true;
        }
        for (; i >= readyStart; i = (i - 1 + readyCapacity) % readyCapacity)
        {
            int prevIndex = (i - 1 + readyCapacity) % readyCapacity;
            readyQueue[i] = readyQueue[prevIndex];
        }
    }
    if (terminated != NULL)
    {
        delete[] terminated->stack;
        terminated = NULL;
    }
    terminated = _thread;
    // dispatch to next thread
    if (!empty)
    {
        yield();
    }
}
