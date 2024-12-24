#include <include/jsonlib/json.h>

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "json_test_allocator.h"

int main()
{
	const char* str = "{\"string\":\"bananas\"}";

	InitTESTAllocatorContext();
	JSONLIB_SetAllocator(TESTAllocate, TESTDeallocate);

	JSON* json = JSONLIB_ParseJSON(str, (u32)strlen(str));

	assert(json != NULL);

	assert(json->valueCount == 1);

	assert(!strcmp(json->values[0]->name, "string"));

	assert(!strcmp(json->values[0]->string, "bananas"));

	const char* jsonStr = JSONLIB_MakeJSON(json, false);

	assert(!strcmp(str, jsonStr));

	TESTDeallocate((void*)jsonStr);

	JSONLIB_FreeJSON(json);

	assert(allocations == 0);

	return 0;
}