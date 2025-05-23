#include <include/jsonlib/json.h>

#include <assert.h>
#include <string.h>

#include "json_test_allocator.h"

int main()
{
	const char* str = "{\"value\":null}";

	InitTESTAllocatorContext();
	JSONLIB_SetAllocator(TESTAllocate, TESTDeallocate);

	JSON* json = JSONLIB_ParseJSON(str, (u32)strlen(str));

	assert(json != NULL);

	assert(json->valueCount == 1);

	assert(!strcmp(json->values[0]->name, "value"));

	assert(json->values[0]->valueCount == 0);

	assert(json->values[0]->values == NULL);

	const char* jsonStr = JSONLIB_MakeJSON(json, false);

	assert(!strcmp(str, jsonStr));

	JSONLIB_ClearJSON((void*)jsonStr);

	JSONLIB_FreeJSON(json);

	assert(allocations == 0);

	return 0;
}