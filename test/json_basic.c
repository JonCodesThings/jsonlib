#include <include/jsonlib/json.h>

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "json_test_allocator.h"

int main()
{
	const char *str = "{}";

	InitAllocatorContext();
	SetJSONAllocator(Allocate, Deallocate);

	JSON *json = ParseJSON(str, (u32)strlen(str));
	
	assert(json != NULL);

	const char* jsonStr = MakeJSON(json, false);

	assert(!strcmp(str, jsonStr));

	Deallocate(jsonStr);

	FreeJSON(json);

	assert(allocations == 0);

	return 0;
}