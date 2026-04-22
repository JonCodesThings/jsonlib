#define JSONLIB_IMPLEMENTATION
#include <include/jsonlib/json.h>

#include <assert.h>
#include <string.h>

#include "../util/json_test_allocator.h"

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

	assert(array->name != NULL);

	assert(!strcmp(array->name, "array"));

	assert(array->values != NULL);

	assert(array->values[0] != NULL);

	assert(array->values[0]->string != NULL);

	assert(!strcmp(array->values[0]->string, "bananas"));

	assert(array->values[1] != NULL);

	assert(array->values[1]->string != NULL);

	assert(!strcmp(array->values[1]->string, "concert"));

	assert(array->values[2] != NULL);

	assert(array->values[2]->string != NULL);

	assert(!strcmp(array->values[2]->string, "hmm"));

	const char* jsonStr = JSONLIB_MakeJSON(json, 0);

	assert(jsonStr != NULL);

	assert(!strcmp(str, jsonStr));

	JSONLIB_ClearJSON((void*)jsonStr);

	JSONLIB_FreeJSON(json);

	assert(allocations == 0);

	return 0;
}