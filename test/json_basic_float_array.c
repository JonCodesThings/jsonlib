#include <include/jsonlib/json.h>

#include <assert.h>
#include <string.h>

#include "json_test_allocator.h"

int main()
{
	const char* str = "{\"array\":[3.14,27.3245,77.43535678]}";

	InitAllocatorContext();
	SetJSONAllocator(Allocate, Deallocate);

	JSON* json = ParseJSON(str, (u32)strlen(str));

	assert(json != NULL);

	assert(json->valueCount == 1);

	JSON* array = json->values[0];

	assert(array->valueCount == 3);

	assert(!strcmp(array->name, "array"));

	const char* jsonStr = MakeJSON(json, false);

	if (strcmp(str, jsonStr))
	{
		FreeJSON(json);
		json = ParseJSON(jsonStr, (u32)strlen(jsonStr));
		array = json->values[0];
		assert(array->values[0]->decimal == 3.14f);
		assert(array->values[1]->decimal == 27.3245f);
		assert(array->values[2]->decimal == 77.43535678f);
	}

	Deallocate(jsonStr);

	FreeJSON(json);

	assert(allocations == 0);

	return 0;
}