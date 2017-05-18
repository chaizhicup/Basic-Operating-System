/*
 File: ContFramePool.C

 Author: Spencer Rawls
 Date  : 2/13/17

 */

/*--------------------------------------------------------------------------*/
/*
 POSSIBLE IMPLEMENTATION
 -----------------------

 The class SimpleFramePool in file "simple_frame_pool.H/C" describes an
 incomplete vanilla implementation of a frame pool that allocates
 *single* frames at a time. Because it does allocate one frame at a time,
 it does not guarantee that a sequence of frames is allocated contiguously.
 This can cause problems.

 The class ContFramePool has the ability to allocate either single frames,
 or sequences of contiguous frames. This affects how we manage the
 free frames. In SimpleFramePool it is sufficient to maintain the free
 frames.
 In ContFramePool we need to maintain free *sequences* of frames.

 This can be done in many ways, ranging from extensions to bitmaps to
 free-lists of frames etc.

 IMPLEMENTATION:

 One simple way to manage sequences of free frames is to add a minor
 extension to the bitmap idea of SimpleFramePool: Instead of maintaining
 whether a frame is FREE or ALLOCATED, which requires one bit per frame,
 we maintain whether the frame is FREE, or ALLOCATED, or HEAD-OF-SEQUENCE.
 The meaning of FREE is the same as in SimpleFramePool.
 If a frame is marked as HEAD-OF-SEQUENCE, this means that it is allocated
 and that it is the first such frame in a sequence of frames. Allocated
 frames that are not first in a sequence are marked as ALLOCATED.

 NOTE: If we use this scheme to allocate only single frames, then all
 frames are marked as either FREE or HEAD-OF-SEQUENCE.

 NOTE: In SimpleFramePool we needed only one bit to store the state of
 each frame. Now we need two bits. In a first implementation you can choose
 to use one char per frame. This will allow you to check for a given status
 without having to do bit manipulations. Once you get this to work,
 revisit the implementation and change it to using two bits. You will get
 an efficiency penalty if you use one char (i.e., 8 bits) per frame when
 two bits do the trick.

 DETAILED IMPLEMENTATION:

 How can we use the HEAD-OF-SEQUENCE state to implement a contiguous
 allocator? Let's look a the individual functions:

 A WORD ABOUT RELEASE_FRAMES():

 When we releae a frame, we only know its frame number. At the time
 of a frame's release, we don't know necessarily which pool it came
 from. Therefore, the function "release_frame" is static, i.e.,
 not associated with a particular frame pool.

 This problem is related to the lack of a so-called "placement delete" in
 C++. For a discussion of this see Stroustrup's FAQ:
 http://www.stroustrup.com/bs_faq2.html#placement-delete

 */

 /*--------------------------------------------------------------------------*/
 /* DEFINES */

#define FREE 0
#define ALLOCATED 1
#define HEAD_OF_SEQUENCE 3
#define OFF_LIMITS 2

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "cont_frame_pool.H"
#include "console.H"
#include "utils.H"
#include "assert.H"

/*--------------------------------------------------------------------------*/
/* DATA STRUCTURES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* CONSTANTS */
/*--------------------------------------------------------------------------*/

ContFramePool* ContFramePool::first = NULL;

/*--------------------------------------------------------------------------*/
/* FORWARDS */
/*--------------------------------------------------------------------------*/

char changeTwoBits(char originalByte, char twoBits, unsigned int offset)
{
    char clearMask = ~(3 << offset);
    originalByte &= clearMask;
    char setMask = twoBits << offset;
    return originalByte | setMask;
}

/*--------------------------------------------------------------------------*/
/* METHODS FOR CLASS   C o n t F r a m e P o o l */
/*--------------------------------------------------------------------------*/

ContFramePool::ContFramePool(unsigned long _base_frame_no,
                             unsigned long _nframes,
                             unsigned long _info_frame_no,
                             unsigned long _n_info_frames)
{
    num_frames = _nframes;
    base_frame_num = _base_frame_no;
    next = NULL;
    unsigned long framesNeededToManage = needed_info_frames(_nframes);
    if (_info_frame_no) // storing managment information externally
    {
      managementHead = (char*)(_info_frame_no * FRAME_SIZE);
    }
    else // storing management information internally
    {
      managementHead = (char*)(_base_frame_no * FRAME_SIZE);
    }
    for (unsigned long i = 0; i < (num_frames >> 2); ++i)
    {
      managementHead[i] = 0;
    }
    if (_info_frame_no == 0)
    {
      mark_inaccessible(base_frame_num, framesNeededToManage, false);
    }
    addToList();
}

void ContFramePool::addToList()
{
    if (first == NULL)
    {
        first = this;
    }
    else
    {
        ContFramePool* iter = first;
        while (iter->next != NULL)
        {
            iter = iter->next;
        }
        iter->next = this;
    }
}

unsigned long ContFramePool::get_frames(unsigned int _n_frames)
{
    unsigned int consecutive_frames = 0, block_start = 0;
    for (unsigned long i = 0; i < num_frames; ++i)
    {
        unsigned int index = i >> 2;
        unsigned int bitshift = (i & 3) << 1;
        char mask = (managementHead[index] >> bitshift) & 3;
        if (mask == FREE)
        {
            if (consecutive_frames == 0)
            {
                block_start = i;
            }
            ++consecutive_frames;
        }
        else
        {
            consecutive_frames = 0;
        }
        if (consecutive_frames >= _n_frames)
        {
	        mark_inaccessible(block_start + base_frame_num, _n_frames, false);
            return block_start + base_frame_num;
        }
    }
    return 0;
}

void ContFramePool::mark_inaccessible(unsigned long _base_frame_no,
                                      unsigned long _n_frames,
                                      bool offLimits)
{
    char firstCode = offLimits ? OFF_LIMITS : HEAD_OF_SEQUENCE;
    char afterCode = offLimits ? OFF_LIMITS : ALLOCATED;
    unsigned long true_base = (_base_frame_no - base_frame_num);
    for (unsigned long i = true_base; i < true_base + _n_frames; ++i)
    {
        unsigned int index = i >> 2;
        unsigned int bitshift = (i & 3) << 1;
        if (i == true_base)
        {
            managementHead[index] = changeTwoBits(managementHead[index], firstCode, bitshift);
        }
        else
        {
            managementHead[index] = changeTwoBits(managementHead[index], afterCode, bitshift);
        }
	}
}

void ContFramePool::release_frame(unsigned long _first_frame_no)
{
    unsigned long true_base = (_first_frame_no - base_frame_num);
    unsigned long index = true_base >> 2;
    unsigned long bitshift = (true_base & 3) << 1;
    char mask = (managementHead[index] >> bitshift) & 3;
    if (mask == HEAD_OF_SEQUENCE)
    {
        // mark it as allocated for now so in the coming loop we don't need to
        // check for two different values
        managementHead[index] = changeTwoBits(managementHead[index], ALLOCATED, bitshift);
    }
    else
    {
        return;
    }
    for (unsigned long i = true_base; i < base_frame_num + num_frames; ++i)
    {
        index = i >> 2;
        bitshift = (i & 3) << 1;
        mask = (managementHead[index] >> bitshift) & 3;
        if (mask == ALLOCATED)
        {
            managementHead[index] = changeTwoBits(managementHead[index], FREE, bitshift);
        }
        else
        {
            break;
        }
	}
}

void ContFramePool::release_frames(unsigned long _first_frame_no)
{
    ContFramePool* iter = first;
    while (iter != NULL)
    {
        if (_first_frame_no >= iter->base_frame_num &&
            _first_frame_no < iter->base_frame_num + iter->num_frames)
        {
            iter->release_frame(_first_frame_no);
            break;
        }
        iter = iter->next;
    }
}

unsigned long ContFramePool::needed_info_frames(unsigned long _n_frames)
{
    // 2 bits per frame state == 4 frame states per byte
    unsigned long rem = _n_frames % (4 * FRAME_SIZE);
    unsigned long quo = _n_frames / (4 * FRAME_SIZE);
    quo += rem ? 1 : 0;
    return quo;
}
