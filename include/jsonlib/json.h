#ifndef JSONLIB_H
#define JSONLIB_H

#ifndef JSONLIB_NO_STDLIB
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
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

typedef struct JSON* JSONptr;

// NOTE: @Jon
// Node in the JSON tree
typedef struct JSON
{
	// Parent links to the parent node of this
	JSONptr parent; //= NULL;

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
		struct JSONptr * values; //= NULL;
	};
} JSON;



// NOTE: @Jon
// Sets the internal allocation functions that the library will use to allocate/free memory
void JSONLIB_SetAllocator(JSONLIB_ALLOC alloc, JSONLIB_DEALLOC dealloc);

// NOTE: @Jon
// Parses a JSON string
// The string need not be null-terminated
JSONptr JSONLIB_ParseJSON(const char *jsonString, u32 stringLength);

// NOTE: @Jon
// Constructs a JSON string from a given tree
// The returnded string will be null-terminated
const char *JSONLIB_MakeJSON(const JSONptr const json, const u8 flags);

// NOTE: @Jon
// Allocates a JSON node
// Uses the allocation functions specified with JSONLIB_SetAllocator
JSONptr  JSONLIB_AllocateJSON(const char* name, struct JSONptr  parent);

// NOTE: @Jon
// Allocates a JSON node
// Uses the allocation functions specified with JSONLIB_SetAllocator
JSONptr  JSONLIB_AllocateIntegerJSON(const char* name, struct JSONptr  parent, const i32 integer);

// NOTE: @Jon
// Allocates a JSON node
// Uses the allocation functions specified with JSONLIB_SetAllocator
JSONptr  JSONLIB_AllocateStringJSON(const char* name, struct JSONptr  parent, const char* string);


// NOTE: @Jon
// Allocates a JSON node
// Uses the allocation functions specified with JSONLIB_SetAllocator
JSONptr  JSONLIB_AllocateBooleanJSON(const char* name, struct JSONptr  parent, const i32 isTrue);

// NOTE: @Jon
// Allocates a JSON node
// Uses the allocation functions specified with JSONLIB_SetAllocator
JSONptr  JSONLIB_AllocateDecimalJSON(const char* name, struct JSONptr  parent, f32 decimal);

// NOTE: @Jon
// Adds a node as a child of another node
void JSONLIB_AddValueJSON(JSONptr json, JSONptr val);

// NOTE: @Jon
// Gets a value by name from a given node
JSONptr JSONLIB_GetValueJSON(const char *name, u32 nameLength, JSONptr json);

// NOTE: @Jon
// Frees memory associated with a given node and all of its children
void JSONLIB_FreeJSON(JSONptr json);

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

	// Digit tokens
	JSONLIB_0 = '0',
	JSONLIB_1 = '1',
	JSONLIB_2 = '2',
	JSONLIB_3 = '3',
	JSONLIB_4 = '4',
	JSONLIB_5 = '5',
	JSONLIB_6 = '6',
	JSONLIB_7 = '7',
	JSONLIB_8 = '8',
	JSONLIB_9 = '9',

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
	u32 inputPos; // = 0;
} JSONLIB_TOKEN;

typedef struct JSONLIB_TOKENS
{
	const char* inputStr;
	JSONLIB_TOKEN *tokens;
	u32 count;
	u32 capacity;
	u32 processed;
} JSONLIB_TOKENS;

static JSONLIB_ALLOC 	JSONLIB_Allocate = malloc;
static JSONLIB_DEALLOC 	JSONLIB_Deallocate = free;

void JSONLIB_SetAllocator(JSONLIB_ALLOC alloc, JSONLIB_DEALLOC dealloc)
{
	JSONLIB_Allocate = alloc;
	JSONLIB_Deallocate = dealloc;
}

void JSONLIB_PushToken(JSONLIB_TOKENS* t, enum JSONLIB_TOKEN_TYPE type, const u32 inputPos)
{
	if (t->count + 1 > t->capacity)
	{
		JSONLIB_TOKEN* newTokens = JSONLIB_Allocate(sizeof(JSONLIB_TOKEN) * t->capacity * 2);
		t->capacity *= 2;
		memcpy(newTokens, t->tokens, sizeof(JSONLIB_TOKEN) * t->count);
		JSONLIB_Deallocate(t->tokens);
		t->tokens = newTokens;
	}

	JSONLIB_TOKEN* token = &t->tokens[t->count++];
	token->type = type;
	token->inputPos = inputPos;
}

JSONLIB_TOKENS JSONLIB_TokeniseString(const char* str, const u32 strLength)
{
	JSONLIB_TOKENS tContainer;
	tContainer.capacity = tContainer.count = 2;
	tContainer.processed = 0;
	tContainer.tokens = JSONLIB_Allocate(sizeof(JSONLIB_TOKEN)* tContainer.capacity);
	tContainer.inputStr = str;

	for (u32 iter = 0; iter < strLength; iter++)
	{
		switch(str[iter])
		{
			case JSONLIB_LEFT_BRACE:
			case JSONLIB_RIGHT_BRACE:
			case JSONLIB_COLON:

			case JSONLIB_LEFT_SQUARE_BRACKET:
			case JSONLIB_RIGHT_SQUARE_BRACKET:
			case JSONLIB_COMMA:

			case JSONLIB_QUOTE:
			case JSONLIB_u:

			case JSONLIB_E:
			case JSONLIB_e:
			case JSONLIB_MINUS:
			case JSONLIB_PLUS:
			case JSONLIB_DOT:

			case JSONLIB_0:
			case JSONLIB_1:
			case JSONLIB_2:
			case JSONLIB_3:
			case JSONLIB_4:
			case JSONLIB_5:
			case JSONLIB_6:
			case JSONLIB_7:
			case JSONLIB_8:
			case JSONLIB_9:

			case JSONLIB_SPACE:
			case JSONLIB_RETURN:
			case JSONLIB_NEWLINE:
			case JSONLIB_TAB:
				JSONLIB_PushToken(&tContainer, str[iter], iter);
				continue;
		}
	}

	return tContainer;
}

void JSONLIB_CleanupContainer(JSONLIB_TOKENS* tContainer)
{
	JSONLIB_Deallocate(tContainer->tokens);
	JSONLIB_Deallocate(tContainer);
}

JSONptr JSONLIB_ParseObject(JSONLIB_TOKENS* tContainer);

void JSONLIB_IgnoreWhitespace(JSONLIB_TOKENS* tContainer)
{
	while (tContainer->processed < tContainer->count)
	{
		const enum JSONLIB_TOKEN_TYPE type = tContainer->tokens[tContainer->processed].type;
		if (type == JSONLIB_SPACE || type == JSONLIB_RETURN 
			|| type == JSONLIB_NEWLINE || type == JSONLIB_TAB)
		{
			tContainer->processed++;
		}
		break;
	}
}

const char* JSONLIB_ParseString(JSONLIB_TOKENS* tContainer)
{
	// Grab our starting token and validate we're parsing a string value
	const JSONLIB_TOKEN* strStart = &tContainer->tokens[tContainer->processed++];
	if (strStart->type != JSONLIB_QUOTE) return NULL;

	// Skip over all non-QUOTE tokens
	const JSONLIB_TOKEN* iter = &tContainer->tokens[tContainer->processed];
	while (iter->type != JSONLIB_QUOTE && tContainer->processed < tContainer->count)
	{
		tContainer->processed++;
		iter = &tContainer->tokens[tContainer->processed];
	}

	// Grab our ending token and validate it matches what we're expecting
	const JSONLIB_TOKEN* strEnd = &tContainer->tokens[tContainer->processed];
	if (strEnd->type != JSONLIB_QUOTE) return NULL;

	// Using the input positions copy the string data over to the JSONLIB heap
	// This string is NULL-terminated hence the additional char allocated
	const u32 parsedStrLength = strEnd->inputPos - strStart->inputPos;
	char* parsedStr = JSONLIB_Allocate(sizeof(char) * parsedStrLength + 1); 
	memcpy(parsedStr, &tContainer->inputStr[strStart->inputPos + 1], sizeof(char) * parsedStrLength);
	parsedStr[parsedStrLength] = '\0';

	return parsedStr;
}

JSONptr JSONLIB_ParseArray(JSONLIB_TOKENS* tContainer)
{
	if (tContainer->tokens[tContainer->processed++].type != JSONLIB_LEFT_SQUARE_BRACKET) return NULL;

	while (tContainer->tokens[tContainer->processed].type != JSONLIB_COMMA && tContainer->tokens[tContainer->processed].type != JSONLIB_RIGHT_SQUARE_BRACKET)
	{
		tContainer->processed++;
		JSONLIB_IgnoreWhitespace(tContainer);

		JSONLIB_ParseValue(tContainer);
	}

	JSONLIB_IgnoreWhitespace(tContainer);

	if (tContainer->tokens[tContainer->processed++].type != JSONLIB_RIGHT_SQUARE_BRACKET) return NULL;
}

JSONptr JSONLIB_ParseValue(JSONLIB_TOKENS* tContainer)
{
	JSONLIB_IgnoreWhitespace(tContainer);

	const enum JSONLIB_TOKEN_TYPE type = tContainer->tokens[tContainer->processed].type;

	switch (type)
	{
		case JSONLIB_LEFT_SQUARE_BRACKET: 
		{
			return JSONLIB_ParseArray(tContainer);
		}
		case JSONLIB_LEFT_BRACE: return JSONLIB_ParseObject(tContainer);
		case JSONLIB_QUOTE:
		{
			const char* str = JSONLIB_ParseString(tContainer);
		}
		case JSONLIB_TRUE:
		case JSONLIB_FALSE:
		case JSONLIB_NULL:
			break;

		default: return NULL;
	}

	JSONLIB_IgnoreWhitespace(tContainer);
}

JSONptr JSONLIB_ParseObject(JSONLIB_TOKENS* tContainer)
{
	// TODO: @Jon
	// Handle dellocation on parsing errors here?
	if (tContainer->tokens[tContainer->processed++].type != JSONLIB_LEFT_BRACE) return NULL;

	JSONptr parsedObj = JSONLIB_Allocate(sizeof(JSON));

	while (tContainer->tokens[tContainer->processed].type != JSONLIB_COMMA && tContainer->tokens[tContainer->processed].type != JSONLIB_RIGHT_BRACE)
	{
		tContainer->processed++;

		JSONLIB_IgnoreWhitespace(tContainer);

		const char* name = JSONLIB_ParseString(tContainer);

		JSONLIB_IgnoreWhitespace(tContainer);

		// TODO: @Jon
		// Handle dellocation on parsing errors here?
		if (tContainer->tokens[tContainer->processed++].type != JSONLIB_COLON) return NULL; 

		JSONLIB_ParseValue(tContainer);
	}
	
	JSONLIB_IgnoreWhitespace(tContainer);

	// TODO: @Jon
	// Handle dellocation on parsing errors here?
	if (tContainer->tokens[tContainer->processed++].type != JSONLIB_RIGHT_BRACE) return NULL;

	return parsedObj;
}

JSONptr JSONLIB_ParseJSON(const char *jsonString, u32 stringLength)
{
	JSONLIB_TOKENS tContainer = JSONLIB_TokeniseString(jsonString, stringLength);
	JSONptr root = JSONLIB_ParseObject(&tContainer);
}

#endif

#endif
