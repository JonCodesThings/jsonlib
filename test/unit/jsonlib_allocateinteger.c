#define JSONLIB_IMPLEMENTATION
#include <include/jsonlib/json.h>

#include <assert.h>
#include <string.h>

#include "../util/json_test_allocator.h"

int main()
{

	InitTESTAllocatorContext();
	JSONLIB_SetAllocator(TESTAllocate, TESTDeallocate);

	JSON *json = JSONLIB_AllocateIntegerJSON(NULL, 32);

    assert(json != NULL);

    assert(json->name == NULL);
    
    assert(json->values == NULL);

    assert(json->valueCount == 0);

    assert(json->tags & JSONLIB_INTEGER_TAG);

    assert(json->integer == 32);

    assert(allocations == 1);

    JSONLIB_FreeJSON(json);
	
	assert(allocations == 0);

	return 0;
}