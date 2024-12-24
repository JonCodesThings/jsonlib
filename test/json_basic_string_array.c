#include <include/jsonlib/json.h>

#include <assert.h>
#include <string.h>

#include "json_test_allocator.h"

int main()
{
	const char* str = "{\"array\":[\"bananas\",\"concert\",\"hmm\"]}";

	InitTESTAllocatorContext();
	JSONLIB_SetAllocator(TESTAllocate, TESTDeallocate);

	JSON* json = JSONLIB_ParseJSON(str, (u32)strlen(str));

	assert(json != NULL);

	assert(json->valueCount == 1);

	JSON* array = json->values[0];

	assert(array->valueCount == 3);

	assert(!strcmp(array->name, "array"));

	assert(!strcmp(array->values[0]->string, "bananas"));

	assert(!strcmp(array->values[1]->string, "concert"));

	assert(!strcmp(array->values[2]->string, "hmm"));

	const char* jsonStr = JSONLIB_MakeJSON(json, false);

	assert(!strcmp(str, jsonStr));

	TESTDeallocate((void*)jsonStr);

	JSONLIB_FreeJSON(json);

	assert(allocations == 0);

	return 0;
}