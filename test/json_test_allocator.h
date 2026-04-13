#ifndef JSONLIB_TEST_ALLOCATOR_H
#define JSONLIB_TEST_ALLOCATOR_H

#include <stdlib.h>

unsigned int allocations;

void InitTESTAllocatorContext()
{
	allocations = 0;
}

void* TESTAllocate(size_t size)
{
	allocations++;
	return malloc(size);
}

void TESTDeallocate(void* ptr)
{
	allocations--;
	free(ptr);
}

#endif