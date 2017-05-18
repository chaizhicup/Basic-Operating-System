#include "assert.H"
#include "exceptions.H"
#include "console.H"
#include "paging_low.H"
#include "page_table.H"
#include "simple_keyboard.H"

PageTable * PageTable::current_page_table = NULL;
unsigned int PageTable::paging_enabled = 0;
ContFramePool * PageTable::kernel_mem_pool = NULL;
ContFramePool * PageTable::process_mem_pool = NULL;
unsigned long PageTable::shared_size = 0;



void PageTable::init_paging(ContFramePool * _kernel_mem_pool,
                            ContFramePool * _process_mem_pool,
                            const unsigned long _shared_size)
{
    kernel_mem_pool = _kernel_mem_pool;
    process_mem_pool = _process_mem_pool;
    shared_size = _shared_size;
    Console::puts("Initialized Paging System\n");
}

PageTable::PageTable()
{
    unsigned long directory_frame = kernel_mem_pool->get_frames(1);
    page_directory = (unsigned long*)(directory_frame * PAGE_SIZE);
    unsigned long* page_table_page = (unsigned long*)
        (kernel_mem_pool->get_frames(1) * PAGE_SIZE);
    page_directory[0] = (unsigned long)page_table_page | 3;
    for (unsigned long i = 0; i < 1024; ++i)
    {
        page_table_page[i] = (i * 4096) | 3;
    }
    for (unsigned long i = 1; i < 1024; ++i)
    {
        page_directory[i] = 2;
    }
    page_directory[1023] = (unsigned long)page_directory | 3;
    Console::puts("Constructed Page Table object\n");
}


void PageTable::load()
{
    current_page_table = this;
    write_cr3((unsigned long)page_directory);
    Console::puts("Loaded page table\n");
}

void PageTable::register_pool(VMPool* pool)
{
    if (first_vm_pool == NULL)
    {
        first_vm_pool = pool;
    }
    else
    {
        VMPool* iter = first_vm_pool;
        while (iter->next != NULL)
        {
            iter = iter->next;
        }
        iter->next = pool;
    }
}

void PageTable::free_page(unsigned long page_no)
{
    unsigned long subpage_no = page_no & 0x3ff;
    unsigned long page_table_no = (page_no >> 10) & 0x3ff;
    unsigned long pte_addr = (1023 << 22) | (page_table_no << 12)
        | (subpage_no << 2);
    unsigned long pte = *(unsigned long*)pte_addr;
    unsigned long frame_no = pte / PAGE_SIZE;
    ContFramePool::release_frames(frame_no);
    *(unsigned long*)pte_addr &= (~1);

    write_cr3(read_cr3());

    Console::puts("Freed page ");
    Console::putui(page_no);
    Console::puts("\n");
}

void PageTable::enable_paging()
{
    write_cr0(read_cr0() | 0x80000000);
    Console::puts("Enabled paging\n");
}

void PageTable::handle_fault(REGS * _r)
{
    unsigned long fault_address = read_cr2();

    bool address_valid = false;
    for (VMPool* pool = current_page_table->first_vm_pool; pool != NULL;
        pool = pool->next)
    {
        if (pool->is_legitimate(fault_address))
        {
            address_valid = true;
            break;
        }
    }
    if (((fault_address >> 22) & 0x3ff) == 0x3ff)
    {
        address_valid = true;
    }
    if (!address_valid)
    {
        Console::puts("Segmentation Fault: ");
        Console::putui(fault_address);
        for (;;);
    }

    unsigned long desired_page = (fault_address >> 12) & 0xfffff;
    unsigned long page_table_page = desired_page >> 10;
    unsigned long page = desired_page & 0x3ff;
    unsigned long* page_table;
    unsigned long* page_addr;

    unsigned long* page_table_page_addr = (unsigned long*)((0x3ff << 22)
        | (0x3ff << 12) | (page_table_page << 2));

    if ((*page_table_page_addr & 1) == 0)
    {
        *page_table_page_addr = (process_mem_pool->get_frames(1) * PAGE_SIZE) | 3;
        page_table = (unsigned long*)(*page_table_page_addr & (~0xfff));
        for (unsigned long i = 0; i < 1024; ++i)
        {
            page_addr = (unsigned long*)((0x3ff << 22)
                | (page_table_page << 12) | (i << 2));
            *page_addr = 2;
        }
    }
    page_table = (unsigned long*)(*page_table_page_addr & (~0xfff));
    unsigned long frame = process_mem_pool->get_frames(1);
    page_addr = (unsigned long*)((0x3ff << 22)
        | (page_table_page << 12) | (page << 2));
    *page_addr = (frame * PAGE_SIZE) | 3;

    Console::puts("Handled page fault\n");
}
