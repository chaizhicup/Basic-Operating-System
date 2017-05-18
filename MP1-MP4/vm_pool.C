/*
 File: vm_pool.C

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

#include "vm_pool.H"
#include "page_table.H"
#include "console.H"
#include "utils.H"
#include "assert.H"
#include "simple_keyboard.H"

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
/* METHODS FOR CLASS   V M P o o l */
/*--------------------------------------------------------------------------*/

VMPool::VMPool(unsigned long  _base_address,
               unsigned long  _size,
               ContFramePool *_frame_pool,
               PageTable     *_page_table) {
    next = NULL;
    _page_table->register_pool(this);
    page_table = _page_table;
    frame_pool = _frame_pool;
    size = _size;
    allocated_list_size = 0;
    allocated_list = (VMBlock*)_base_address;
    base_address = _base_address;
    Console::puts("Constructed VMPool object.\n");
}

unsigned long VMPool::find_free_pages(unsigned long num_pages)
{
    unsigned long prev_first_free = 0;
    unsigned long max_page_no = (base_address + size) / PageTable::PAGE_SIZE;
    unsigned long first_free = base_address / PageTable::PAGE_SIZE + 1;
    while (prev_first_free != first_free && first_free < max_page_no)
    {
        prev_first_free = first_free;
        for (unsigned long i = 0; i < allocated_list_size; ++i)
        {
            if (first_free + num_pages > allocated_list[i].first_page)
            {
                first_free = allocated_list[i].first_page + allocated_list[i].size;
            }
        }
    }
    if (first_free >= max_page_no)
    {
        return 0;
    }
    else
    {
        return first_free;
    }
}

unsigned long VMPool::allocate(unsigned long _size) {
    unsigned long num_pages = _size / PageTable::PAGE_SIZE;
    unsigned long page_no = find_free_pages(num_pages);
    if (page_no != 0)
    {
        allocated_list[allocated_list_size].first_page = page_no;
        allocated_list[allocated_list_size].size = _size;
        ++allocated_list_size;
    }
    Console::puts("Allocated memory:");
    Console::putui(page_no);
    Console::puts("-");
    Console::putui(page_no + num_pages);
    Console::puts("\n");
    return page_no * PageTable::PAGE_SIZE;
}

void VMPool::remove_allocated_at(unsigned long index)
{
    for (unsigned long i = index; i < allocated_list_size - 1; ++i)
    {
        allocated_list[i] = allocated_list[i + 1];
    }
    --allocated_list;
}

void VMPool::release(unsigned long _start_address) {
    unsigned long page_no = _start_address / PageTable::PAGE_SIZE;
    for (unsigned long i = 0; i < allocated_list_size; ++i)
    {
        if (allocated_list[i].first_page == page_no)
        {
            for (unsigned long j = page_no; j < allocated_list[i].size; ++j)
            {
                page_table->free_page(j);
            }
            remove_allocated_at(i);
            Console::puts("Released region of memory.\n");
            return;
        }
    }
    Console::puts("Invalid region of memory: ");
    Console::putui(_start_address);
    Console::puts("\n");
}

bool VMPool::is_legitimate(unsigned long _address) {
    if (_address < base_address || _address >= base_address + size)
    {
        Console::puts("Address is not within range managed by this pool\n");
        return false;
    }
    if (_address / PageTable::PAGE_SIZE == base_address / PageTable::PAGE_SIZE)
    {
        Console::puts("Address is within management page\n");
        return true;
    }
    for (int i = 0; i < allocated_list_size; ++i)
    {
        if (_address >= allocated_list[i].first_page * PageTable::PAGE_SIZE &&
            _address < (allocated_list[i].first_page + allocated_list[i].size) * PageTable::PAGE_SIZE)
        {
            Console::puts("Address is within allocated segment\n");
            return true;
        }
    }
    return false;
    Console::puts("This will never be printed\n");
    Console::puts("Checked whether address is part of an allocated region.\n");
}
