#define JSONLIB_IMPLEMENTATION
#include <include/jsonlib/json.h>

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "json_test_allocator.h"

int main()
{
	const char *str = "{";

	InitTESTAllocatorContext();
	JSONLIB_SetAllocator(TESTAllocate, TESTDeallocate);

	JSON *json = JSONLIB_ParseJSON(str, (u32)strlen(str));
	
	assert(json == NULL);

	assert(allocations == 0);

	return 0;
}