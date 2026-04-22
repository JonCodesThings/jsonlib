#define JSONLIB_IMPLEMENTATION
#include <include/jsonlib/json.h>

#include <assert.h>
#include <string.h>

#include "../util/json_test_allocator.h"

int main()
{
	const char* str = "[\"bananas\",\"concert\",\"hmm\"]";

	InitTESTAllocatorContext();
	JSONLIB_SetAllocator(TESTAllocate, TESTDeallocate);

	JSON* json = JSONLIB_ParseJSON(str, (u32)strlen(str));

	assert(json != NULL);

	assert(json->name == NULL);

	assert(json->valueCount == 3);

	assert(json->values != NULL);

	assert(json->values[0] != NULL);

	assert(json->values[0]->string != NULL);

	assert(!strcmp(json->values[0]->string, "bananas"));

	assert(json->values[1] != NULL);

	assert(json->values[1]->string != NULL);

	assert(!strcmp(json->values[1]->string, "concert"));

	assert(json->values[2] != NULL);

	assert(json->values[2]->string != NULL);

	assert(!strcmp(json->values[2]->string, "hmm"));

	const char* jsonStr = JSONLIB_MakeJSON(json, 0);

	assert(jsonStr != NULL);

	assert(!strcmp(str, jsonStr));

	JSONLIB_ClearJSON((void*)jsonStr);

	JSONLIB_FreeJSON(json);

	assert(allocations == 0);

	return 0;
}