#include <include/jsonlib/json.h>

#include <assert.h>
#include <string.h>

#include "json_test_allocator.h"

int main()
{
	const char* str = "{\"float\":0.14}";

	InitTESTAllocatorContext();
	JSONLIB_SetAllocator(TESTAllocate, TESTDeallocate);

	JSON* json = JSONLIB_ParseJSON(str, (u32)strlen(str));

	assert(json != NULL);

	assert(json->valueCount == 1);

	assert(!strcmp(json->values[0]->name, "float"));

	assert(json->values[0]->decimal == 0.14f);

	const char* jsonStr = JSONLIB_MakeJSON(json, false);

	if (strcmp(str, jsonStr))
	{
		JSONLIB_FreeJSON(json);
		json = JSONLIB_ParseJSON(jsonStr, (u32)strlen(jsonStr));
		assert(json->values[0]->decimal == 0.14f);
	}

	JSONLIB_ClearJSON((void*)jsonStr);

	JSONLIB_FreeJSON(json);

	assert(allocations == 0);

	return 0;
}