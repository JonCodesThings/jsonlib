#ifndef JSONLIB_H
#define JSONLIB_H

#ifndef JSONLIB_NO_STDLIB
#include <stddef.h>
#include <stdlib.h>
#endif

#ifdef JSONLIB_HEADER
// TODO: @Jon
// Add more platforms/compilers to this as needed
#ifdef OSLIB_PLATFORM_HEADER
#include <include/oslib/platform.h>
#else
#ifndef NULL
#define NULL 0
#endif
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
#elif __GNUC__
#if __x86_64__
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

// NOTE: @Jon
// Function pointer typedefs for allocation functions
typedef void* (*JSONLIB_ALLOC)(size_t numBytes);
typedef void(*JSONLIB_DEALLOC)(void* bytes);

// NOTE: @Jon
// Constants used for tagging nodes to indicate what type they are representing
extern const u8 JSONLIB_STRING_TAG;
extern const u8 JSONLIB_INTEGER_TAG;
extern const u8 JSONLIB_DECIMAL_TAG;
extern const u8 JSONLIB_ARRAY_TAG;
extern const u8 JSONLIB_OBJECT_TAG;
extern const u8 JSONLIB_BOOLEAN_TAG;
extern const u8 JSONLIB_NULL_TAG;


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

		// Values stores a flat array of values, with valueCount keeping track of how many there are
		struct JSON** values; //= NULL;
	};
} JSON;


// NOTE: @Jon
// Sets the internal allocation functions that the library will use to allocate/free memory
void JSONLIB_SetAllocator(JSONLIB_ALLOC alloc, JSONLIB_DEALLOC dealloc);

// NOTE: @Jon
// Parses a JSON string
JSON *JSONLIB_ParseJSON(const char *jsonString, u32 stringLength);

// NOTE: @Jon
// Constructs a JSON string from a given tree
const char *JSONLIB_MakeJSON(const JSON *const json, const u8 flags);

// NOTE: @Jon
// Allocates a JSON node
// Uses the allocation functions specified with JSONLIB_SetAllocator
JSON* JSONLIB_AllocateJSON(const char* name, struct JSON* parent);

// NOTE: @Jon
// Allocates a JSON node
// Uses the allocation functions specified with JSONLIB_SetAllocator
JSON* JSONLIB_AllocateIntegerJSON(const char* name, struct JSON* parent, const i32 integer);

// NOTE: @Jon
// Allocates a JSON node
// Uses the allocation functions specified with JSONLIB_SetAllocator
JSON* JSONLIB_AllocateStringJSON(const char* name, struct JSON* parent, const char* string);


// NOTE: @Jon
// Allocates a JSON node
// Uses the allocation functions specified with JSONLIB_SetAllocator
JSON* JSONLIB_AllocateBooleanJSON(const char* name, struct JSON* parent, const i32 isTrue);

// NOTE: @Jon
// Allocates a JSON node
// Uses the allocation functions specified with JSONLIB_SetAllocator
JSON* JSONLIB_AllocateDecimalJSON(const char* name, struct JSON* parent, f32 decimal);

// NOTE: @Jon
// Adds a node as a child of another node
void JSONLIB_AddValueJSON(JSON *json, JSON *val);

// NOTE: @Jon
// Gets a value by name from a given node
JSON *JSONLIB_GetValueJSON(const char *name, u32 nameLength, JSON *json);

// NOTE: @Jon
// Frees memory associated with a given node and all of its children
void JSONLIB_FreeJSON(JSON *json);

// NOTE: @Jon
// Frees memory associated with a given string
// N.B. This will set the str pointer to NULL
void JSONLIB_ClearJSON(const char *str);
#endif

#ifdef JSONLIB_IMPLEMENTATION

// TODO: @Jon
// Big TODO list for this file:
//  - Add convenience functions for checking if a JSON struct contains a value of given type
//  - Add proper tree searching (possible with some [] operator overloading? Opt-in C++ support would need to be added)
//	- Better commentary
//  - Way of enforcing precision of floating point string outputs
//  - Make C standard library dependencies optional (allow for user-provided alternatives to these functions)

// NOTE: @Jon
// Tags for JSON nodes
const u8 JSONLIB_STRING_TAG = 1 << 0;
const u8 JSONLIB_INTEGER_TAG = 1 << 1;
const u8 JSONLIB_DECIMAL_TAG = 1 << 2;
const u8 JSONLIB_ARRAY_TAG = 1 << 3;
const u8 JSONLIB_OBJECT_TAG = 1 << 4;
const u8 JSONLIB_BOOLEAN_TAG = 1 << 5;
const u8 JSONLIB_NULL_TAG = 1 << 6;

// NOTE: @Jon
// Some useful strings
static const char* const JSONLIBtrueStr = "true";
static const char* const JSONLIBfalseStr = "false";
static const char* const JSONLIBnullStr = "null";

// NOTE: @Jon
// A struct for handling a char * string
typedef struct JSONLIB_STRING_STRUCT
{
	char *raw;
	u32 length;
	u32 capacity;
} JSONLIB_STRING_STRUCT;

// NOTE: @Jon
// JSON Token Types
// We're taking advantage of only supporting UTF-8 chars for some of the values
enum JSONLIB_TOKEN_TYPE
{
	// JSON constant tokens
	JSONLIB_NULL = 0,
	JSONLIB_TRUE,
	JSONLIB_FALSE,

	// Object tokens
	JSONLIB_LEFT_BRACE = '{',
	JSONLIB_RIGHT_BRACE = '}',
	JSONLIB_COLON = ':',

	// Array tokens
	JSONLIB_LEFT_SQUARE_BRACKET = '[',
	JSONLIB_RIGHT_SQUARE_BRACKET = ']',
	JSONLIB_COMMA = ',',

	// String tokens
	JSONLIB_QUOTE = '"',
	JSONLIB_u = 'u', // used for hexadecimal parsing

	// Number tokens (excluding digits)
	JSONLIB_E = 'E',
	JSONLIB_e = 'e',
	JSONLIB_MINUS = '-',
	JSONLIB_PLUS = '+',
	JSONLIB_DOT = '.',

	// Whitespace tokens
	JSONLIB_SPACE 	= ' ',
	JSONLIB_RETURN 	= '\r',
	JSONLIB_NEWLINE = '\n',
	JSONLIB_TAB 	= '\t',


	JSONLIB_EOF,
	JSONLIB_ERROR
};

// NOTE: @Jon
// Struct to store token information for the JSON
typedef struct JSONLIB_TOKEN
{
	enum JSONLIB_TOKEN_TYPE type; // = JSONLIB_TOKEN_TYPE::JSONLIB_NULL;
	char* str; // = NULL;
	u32 inputPos; // = 0;
} JSONLIB_TOKEN;

typedef struct JSONLIB_TOKENS
{
	JSONLIB_TOKEN *tokens;
	u32 tokenCount;
	u32 tokenCapacity;
} JSONLIB_TOKENS;

static JSONLIB_ALLOC 	JSONLIB_Allocate = malloc;
static JSONLIB_DEALLOC 	JSONLIB_Deallocate = free;

void JSONLIB_SetAllocator(JSONLIB_ALLOC alloc, JSONLIB_DEALLOC dealloc)
{
	JSONLIB_Allocate = alloc;
	JSONLIB_Deallocate = dealloc;
}

#endif

#endif
