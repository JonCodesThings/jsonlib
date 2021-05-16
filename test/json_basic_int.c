#include <include/jsonlib/json.h>

#include <assert.h>
#include <string.h>

#include "json_test_allocator.h"

int main()
{
	const char* str = "{\"int\":32}";

	InitAllocatorContext();
	JSONLIB_SetAllocator(Allocate, Deallocate);

	JSON* json = JSONLIB_ParseJSON(str, (u32)strlen(str));

	assert(json != NULL);

	assert(json->valueCount == 1);

	assert(!strcmp(json->values[0]->name, "int"));

	assert(json->values[0]->integer == 32);

	const char* jsonStr = JSONLIB_MakeJSON(json, false);

	assert(!strcmp(str, jsonStr));

	Deallocate(jsonStr);

	JSONLIB_FreeJSON(json);

	assert(allocations == 0);

	return 0;
}