#ifndef JSONLIB_TEST_ALLOCATOR_H
#define JSONLIB_TEST_ALLOCATOR_H

#include <stdlib.h>

u32 allocations;

void InitAllocatorContext()
{
	allocations = 0;
}

void* Allocate(size_t size)
{
	allocations++;
	return malloc(size);
}

void Deallocate(void* ptr)
{
	allocations--;
	free(ptr);
}

#endif