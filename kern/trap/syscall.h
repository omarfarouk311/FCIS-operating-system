#ifndef FOS_KERN_SYSCALL_H
#define FOS_KERN_SYSCALL_H
#ifndef FOS_KERNEL
# error "This is a FOS kernel header; user programs should not #include it"
#endif

#include <inc/syscall.h>

uint32 syscall(uint32 num, uint32 a1, uint32 a2, uint32 a3, uint32 a4, uint32 a5);

struct BlockMetaData
{
	uint32 size;		//block size (including size of its meta data)
	uint8 is_free;		//is_free block?
	LIST_ENTRY(BlockMetaData) prev_next_info;	/* linked list links */

};


#endif /* !FOS_KERN_SYSCALL_H */
