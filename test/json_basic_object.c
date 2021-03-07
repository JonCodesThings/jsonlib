#include <include/jsonlib/json.h>

#include <assert.h>
#include <string.h>

#include "json_test_allocator.h"

int main()
{
	const char* str = "{\"object\":{\"value\":12}}";

	InitAllocatorContext();
	SetJSONAllocator(Allocate, Deallocate);

	JSON* json = ParseJSON(str, (u32)strlen(str));

	assert(json != NULL);

	assert(json->valueCount == 1);

	assert(json->values[0]->tags & JSON_OBJECT_TAG);

	JSON* object = json->values[0];

	assert(object->valueCount == 1);

	assert(object->tags & JSON_OBJECT_TAG);

	assert(!strcmp(object->name, "object"));

	JSON* value = object->values[0];

	assert(value->tags & JSON_INTEGER_TAG);

	assert(!strcmp(value->name, "value"));

	assert(value->integer == 12);

	const char* jsonStr = MakeJSON(json, false);

	assert(!strcmp(str, jsonStr));

	Deallocate(jsonStr);

	FreeJSON(json);

	assert(allocations == 0);

	return 0;
}