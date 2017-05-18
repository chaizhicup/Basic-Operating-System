/*
 File: kernel.C

 Author: R. Bettati
 Department of Computer Science
 Texas A&M University
 Date  : 12/09/03


 This file has the main entry point to the operating system.

 */


/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "machine.H"     /* LOW-LEVEL STUFF   */
#include "console.H"
#include "gdt.H"
#include "idt.H"          /* LOW-LEVEL EXCEPTION MGMT. */
#include "irq.H"
#include "exceptions.H"
#include "interrupts.H"

#include "simple_keyboard.H" /* SIMPLE KB DRIVER */
#include "simple_timer.H" /* TIMER MANAGEMENT */

#include "page_table.H"
#include "paging_low.H"

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

#define MB * (0x1 << 20)
#define KB * (0x1 << 10)
#define KERNEL_POOL_START_FRAME ((2 MB) / Machine::PAGE_SIZE)
#define KERNEL_POOL_SIZE ((2 MB) / Machine::PAGE_SIZE)
#define PROCESS_POOL_START_FRAME ((4 MB) / Machine::PAGE_SIZE)
#define PROCESS_POOL_SIZE ((28 MB) / Machine::PAGE_SIZE)
/* definition of the kernel and process memory pools */

#define MEM_HOLE_START_FRAME ((15 MB) / Machine::PAGE_SIZE)
#define MEM_HOLE_SIZE ((1 MB) / Machine::PAGE_SIZE)
/* we have a 1 MB hole in physical memory starting at address 15 MB */

#define FAULT_ADDR (4 MB)
/* used in the code later as address referenced to cause page faults. */
#define NACCESS ((1 MB) / 4)
/* NACCESS integer access (i.e. 4 bytes in each access) are made starting at address FAULT_ADDR */

/*--------------------------------------------------------------------------*/
/* MAIN ENTRY INTO THE OS */
/*--------------------------------------------------------------------------*/

int main() {

    GDT::init();
    Console::init();
    SimpleKeyboard::init();

    ContFramePool kernel_mem_pool(KERNEL_POOL_START_FRAME,
                                  KERNEL_POOL_SIZE,
                                  0,
                                  0);
    Console::puts("Initialized kernel frame pool\n");

    unsigned long n_info_frames = ContFramePool::needed_info_frames(PROCESS_POOL_SIZE);
    unsigned long process_mem_pool_info_frame = kernel_mem_pool.get_frames(n_info_frames);
    ContFramePool process_mem_pool(PROCESS_POOL_START_FRAME,
                                   PROCESS_POOL_SIZE,
                                   process_mem_pool_info_frame,
				                   n_info_frames);

    unsigned long test_frame = kernel_mem_pool.get_frames(1);
    Console::putui(test_frame);
    Console::puts("\n");

    test_frame = kernel_mem_pool.get_frames(3);
    Console::putui(test_frame);
    Console::puts("\n");

    test_frame = kernel_mem_pool.get_frames(2);
    Console::putui(test_frame);
    Console::puts("\n");

    /* Take care of the hole in the memory. */
    process_mem_pool.mark_inaccessible(MEM_HOLE_START_FRAME, MEM_HOLE_SIZE);

    test_frame = process_mem_pool.get_frames(1);
    Console::putui(test_frame);
    Console::puts("\n");

    unsigned long test_frame2 = process_mem_pool.get_frames((15 MB) / Machine::PAGE_SIZE);
    Console::putui(test_frame2);
    Console::puts("\n");

    test_frame2 = process_mem_pool.get_frames(4);
    Console::putui(test_frame2);
    Console::puts("\n");

    ContFramePool::release_frames(test_frame);
    test_frame = process_mem_pool.get_frames(2);
    Console::putui(test_frame);
    Console::puts("\n");

    ContFramePool::release_frames(test_frame2);
    test_frame2 = process_mem_pool.get_frames(5);
    Console::putui(test_frame2);
    Console::puts("\n");

    for(;;);

    /* -- WE DO THE FOLLOWING TO KEEP THE COMPILER HAPPY. */
    return 1;
}
