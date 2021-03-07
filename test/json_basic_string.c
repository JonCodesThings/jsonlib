#include <include/jsonlib/json.h>

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "json_test_allocator.h"

int main()
{
	const char* str = "{\"string\":\"bananas\"}";

	InitAllocatorContext();
	SetJSONAllocator(Allocate, Deallocate);

	JSON* json = ParseJSON(str, (u32)strlen(str));

	assert(json != NULL);

	assert(json->valueCount == 1);

	assert(!strcmp(json->values[0]->name, "string"));

	assert(!strcmp(json->values[0]->string, "bananas"));

	const char* jsonStr = MakeJSON(json, false);

	assert(!strcmp(str, jsonStr));

	Deallocate(jsonStr);

	FreeJSON(json);

	assert(allocations == 0);

	return 0;
}