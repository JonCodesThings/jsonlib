#ifndef JSONLIB_H
#define JSONLIB_H

// TODO: @Jon
// Figure if we really need this include here
#include <stdbool.h>

// NOTE: @Jon
// Useful typedefs

// TODO: @Jon
// Add more platforms/compilers to this as needed
#if OSLIB
#include <include/oslib/platform.h>
#else
#ifdef _MSC_VER
#if _WIN32
typedef char i8;
typedef short i16;
typedef int i32;
typedef long long i64;

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;

typedef float f32;
typedef double f64;
#endif
#endif
#endif

#define NULL 0

// NOTE: @Jon
// Function pointer typedefs for allocation functions
typedef void* (*JSON_ALLOC)(size_t);
typedef void(*JSON_DEALLOC)(void*);

// NOTE: @Jon
// Constants used for tagging nodes to indicate what type they are representing
extern const u8 JSON_STRING_TAG;
extern const u8 JSON_INTEGER_TAG;
extern const u8 JSON_DECIMAL_TAG;
extern const u8 JSON_ARRAY_TAG;
extern const u8 JSON_OBJECT_TAG;
extern const u8 JSON_BOOLEAN_TAG;
extern const u8 JSON_NULL_TAG;


// NOTE: @Jon
// Node in the JSON tree
typedef struct JSON
{
	// Parent links to the parent node of this
	struct JSON* parent; //= NULL;

	u32 valueCount; //= 0;

	//The name for the node
	const char* name; //= "";

	// Tags for what the data actually contains
	u8 tags; //= 0;

	// A representation of what type of number/string this node contains as a value (if it isn't an object or an array)
	union
	{
		i32 integer;
		f32 decimal;
		const char *string;
		bool boolean;

		// Values stores a flat array of values, with valueCount keeping track of how many there are
		struct JSON** values; //= NULL;
	};
} JSON;


// NOTE: @Jon
// Sets the internal allocation functions that the library will use to allocate/free memory
void JSONLIB_SetAllocator(JSON_ALLOC alloc, JSON_DEALLOC dealloc);

// NOTE: @Jon
// Parses a JSON string
JSON *JSONLIB_ParseJSON(const char *jsonString, u32 stringLength);

// NOTE: @Jon
// Constructs a JSON string from a given tree
const char *JSONLIB_MakeJSON(const JSON *const json, const bool humanReadable);

// NOTE: @Jon
// Adds a node as a child of another node
void JSONLIB_AddValueJSON(JSON *json, JSON *val);

// NOTE: @Jon
// Gets a value by name from a given node
JSON *JSONLIB_GetValueJSON(const char *name, u32 nameLength, JSON *json);

// NOTE: @Jon
// Frees memory associated with a given node and all of its children
void JSONLIB_FreeJSON(JSON *json);

#endif