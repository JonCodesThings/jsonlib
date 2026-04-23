#define JSONLIB_IMPLEMENTATION
#include <include/jsonlib/json.h>

#include <assert.h>
#include <string.h>

#include "../util/json_test_allocator.h"

int main()
{

	InitTESTAllocatorContext();
	JSONLIB_SetAllocator(TESTAllocate, TESTDeallocate);

	JSON *json = JSONLIB_AllocateDecimalJSON(NULL, 4859.3f);

    assert(json != NULL);

    assert(json->name == NULL);
    
    assert(json->valueCount == 0);

    assert(json->tags & JSONLIB_DECIMAL_TAG);

    assert(json->decimal == 4859.3f);

    assert(allocations == 1);

    JSONLIB_FreeJSON(json);
	
	assert(allocations == 0);

	return 0;
}