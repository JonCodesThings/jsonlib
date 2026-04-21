#ifndef JSONLIB_H
#define JSONLIB_H

#if defined(JSONLIB_HEADER) || defined(JSONLIB_IMPLEMENTATION)
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

#ifndef JSONLIB_NO_STDLIB
#include <stddef.h>
typedef size_t JSONLIB_size_t;
#else
#if __x86_64__
typedef u64 JSONLIB_size_t;
#else
typedef u32 JSONLIB_size_t;
#endif
#endif

#if __x86_64__
typedef u64 JSONLIB_uint_t;
typedef i64 JSONLIB_int_t;
typedef f64 JSONLIB_float_t;
#else
typedef u32 JSONLIB_uint_t;
typedef i32 JSONLIB_int_t;
typedef f32 JSONLIB_float_t;
#endif

// NOTE: @Jon
// Function pointer typedefs for allocation functions
typedef void* (*JSONLIB_ALLOC)(JSONLIB_size_t numBytes);
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

	JSONLIB_size_t valueCount; //= 0;

	//The name for the node
	const char* name; //= "";

	// Tags for what the data actually contains
	u8 tags; //= 0;

	// A representation of what type of number/string this node contains as a value (if it isn't an object or an array)
	union
	{
		JSONLIB_int_t integer;
		JSONLIB_float_t decimal;
		const char *string;

		// Values stores a flat array of values, with valueCount keeping track of how many there are
		JSONptr *values; //= NULL;
	};
} JSON;

extern const u8 JSONLIB_STRING_TAG;
extern const u8 JSONLIB_INTEGER_TAG;
extern const u8 JSONLIB_DECIMAL_TAG;
extern const u8 JSONLIB_ARRAY_TAG;
extern const u8 JSONLIB_OBJECT_TAG;
extern const u8 JSONLIB_TRUE_TAG;
extern const u8 JSONLIB_FALSE_TAG;
extern const u8 JSONLIB_NULL_TAG;

// NOTE: @Jon
// Sets the internal allocation functions that the library will use to allocate/free memory
void JSONLIB_SetAllocator(JSONLIB_ALLOC alloc, JSONLIB_DEALLOC dealloc);

// NOTE: @Jon
// Parses a JSON string
// The string need not be null-terminated
JSONptr JSONLIB_ParseJSON(const char *jsonString, JSONLIB_size_t stringLength);

// NOTE: @Jon
// Constructs a JSON string from a given tree
// The returnded string will be null-terminated
const char *JSONLIB_MakeJSON(const JSONptr json, const u8 flags);

// NOTE: @Jon
// Allocates a JSON node
// Uses the allocation functions specified with JSONLIB_SetAllocator
JSONptr  JSONLIB_AllocateJSON(const char* name, JSONptr parent, const u8 tags);

// NOTE: @Jon
// Allocates a JSON node
// Uses the allocation functions specified with JSONLIB_SetAllocator
JSONptr  JSONLIB_AllocateIntegerJSON(const char* name, JSONptr parent, const JSONLIB_int_t integer);

// NOTE: @Jon
// Allocates a JSON node
// Uses the allocation functions specified with JSONLIB_SetAllocator
JSONptr  JSONLIB_AllocateStringJSON(const char* name, JSONptr parent, const char* string);

// NOTE: @Jon
// Allocates a JSON node
// Uses the allocation functions specified with JSONLIB_SetAllocator
JSONptr  JSONLIB_AllocateDecimalJSON(const char* name, JSONptr parent, JSONLIB_float_t decimal);

// NOTE: @Jon
// Adds a node as a child of another node
void JSONLIB_AddValueJSON(JSONptr json, JSONptr val);

// NOTE: @Jon
// Gets a value by name from a given node
JSONptr JSONLIB_GetValueJSON(const char *name, JSONLIB_size_t nameLength, JSONptr json);

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
const u8 JSONLIB_TRUE_TAG = 1 << 5;
const u8 JSONLIB_FALSE_TAG = 1 << 6;
const u8 JSONLIB_NULL_TAG = 1 << 7;

// NOTE: @Jon
// A struct for handling a char * string
typedef struct JSONLIB_STRING_STRUCT
{
	char *raw;
	JSONLIB_size_t length;
	JSONLIB_size_t capacity;
} JSONLIB_STRING_STRUCT;

// NOTE: @Jon
// JSON Token Types
// We're taking advantage of only supporting UTF-8 chars for some of the values
enum JSONLIB_TOKEN_TYPE
{
	// JSON constant tokens
	JSONLIB_NULL = 0,
	JSONLIB_FALSE,
	JSONLIB_TRUE,

	// Object tokens
	JSONLIB_LEFT_BRACE = '{',
	JSONLIB_RIGHT_BRACE = '}',
	JSONLIB_COLON = ':',

	// Array tokens
	JSONLIB_LEFT_SQUARE_BRACKET = '[',
	JSONLIB_RIGHT_SQUARE_BRACKET = ']',
	JSONLIB_COMMA = ',',

	// Value tokens
	JSONLIB_t = 't',
	JSONLIB_r = 'r',
	JSONLIB_f = 'f',
	JSONLIB_a = 'a',
	JSONLIB_l = 'l',
	JSONLIB_s = 's',
	JSONLIB_n = 'n',

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

	JSONLIB_ERROR
};

static enum JSONLIB_TOKEN_TYPE JSONLIB_TOKEN_TRUE_PATTERN[] 	= { JSONLIB_t, JSONLIB_r, JSONLIB_u, JSONLIB_e };
static enum JSONLIB_TOKEN_TYPE JSONLIB_TOKEN_FALSE_PATTERN[] 	= { JSONLIB_f, JSONLIB_a, JSONLIB_l, JSONLIB_s, JSONLIB_e };
static enum JSONLIB_TOKEN_TYPE JSONLIB_TOKEN_NULL_PATTERN[] 	= { JSONLIB_n, JSONLIB_u, JSONLIB_l, JSONLIB_l };

// NOTE: @Jon
// Struct to store token information for the JSON
typedef struct JSONLIB_TOKEN
{
	enum JSONLIB_TOKEN_TYPE type; // = JSONLIB_TOKEN_TYPE::JSONLIB_NULL;
	JSONLIB_size_t inputPos; // = 0;
} JSONLIB_TOKEN;

typedef struct JSONLIB_TOKENS
{
	const char* inputStr;
	JSONLIB_TOKEN *tokens;
	JSONLIB_size_t count;
	JSONLIB_size_t capacity;
	JSONLIB_size_t processed;
} JSONLIB_TOKENS;

#ifndef JSONLIB_NO_STDLIB
#include <stdlib.h>
#include <stdio.h>
#endif

static JSONLIB_ALLOC 	JSONLIB_Allocate = malloc;
static JSONLIB_DEALLOC 	JSONLIB_Deallocate = free;

void JSONLIB_SetAllocator(JSONLIB_ALLOC alloc, JSONLIB_DEALLOC dealloc)
{
	JSONLIB_Allocate = alloc;
	JSONLIB_Deallocate = dealloc;
}

void JSONLIB_PushToken(JSONLIB_TOKENS* t, enum JSONLIB_TOKEN_TYPE type, const JSONLIB_size_t inputPos)
{
	if (t->count + 1 > t->capacity)
	{
		JSONLIB_TOKEN* newTokens = JSONLIB_Allocate(sizeof(JSONLIB_TOKEN) * t->capacity * 2);
		t->capacity *= 2;

		for (JSONLIB_size_t copyIter = 0; copyIter < t->count; copyIter++)
		{
			newTokens[copyIter].type = t->tokens[copyIter].type;
			newTokens[copyIter].inputPos = t->tokens[copyIter].inputPos;
		}

		JSONLIB_Deallocate(t->tokens);
		t->tokens = newTokens;
	}

	JSONLIB_TOKEN* token = &t->tokens[t->count++];
	token->type = type;
	token->inputPos = inputPos;
}

JSONLIB_TOKEN* JSONLIB_CheckTokenPattern(JSONLIB_TOKENS* container, enum JSONLIB_TOKEN_TYPE patternType, enum JSONLIB_TOKEN_TYPE* pattern, const JSONLIB_size_t patternLength)
{
	if (pattern == NULL)
	{
		return NULL;
	}

	if (container->count < patternLength)
	{
		return NULL;
	}

	const JSONLIB_size_t startToken = container->count - patternLength;
	for (JSONLIB_size_t iter = 0; iter < patternLength; iter++)
	{
		const JSONLIB_TOKEN* token = &container->tokens[startToken + iter];
		if (token->type != pattern[iter])
		{
			return NULL;
		}
	}

	// Set the token count to the start token
	container->count -= patternLength;
	container->tokens[startToken].type = patternType;
	container->count++;
	return &container->tokens[startToken];
}

void JSONLIB_TokenPatternMatch(JSONLIB_TOKENS* container)
{
	const enum JSONLIB_TOKEN_TYPE lastTokenType = container->tokens[container->count - 1].type;

	switch (lastTokenType)
	{
		default: return;
		case JSONLIB_l:
		{
			JSONLIB_TOKEN* nullPatternStart = JSONLIB_CheckTokenPattern(container, JSONLIB_NULL, JSONLIB_TOKEN_NULL_PATTERN, 4);
			if (nullPatternStart != NULL)
			{
				return;
			}
			break;
		}
		case JSONLIB_e:
		{
			JSONLIB_TOKEN* patternStart = JSONLIB_CheckTokenPattern(container, JSONLIB_TRUE, JSONLIB_TOKEN_TRUE_PATTERN, 4);
			if (patternStart != NULL)
			{
				return;
			}

			JSONLIB_CheckTokenPattern(container, JSONLIB_FALSE, JSONLIB_TOKEN_FALSE_PATTERN, 5);
			break;
		}
	}
}

JSONLIB_TOKENS JSONLIB_TokeniseString(const char* str, const JSONLIB_size_t strLength)
{
	JSONLIB_TOKENS container;
	container.count = 0;
	container.capacity = 2;
	container.processed = 0;
	container.tokens = JSONLIB_Allocate(sizeof(JSONLIB_TOKEN)* container.capacity);
	container.inputStr = str;

	for (JSONLIB_size_t iter = 0; iter < strLength; iter++)
	{
		switch(str[iter])
		{
			default: continue;

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

			case JSONLIB_t:
			case JSONLIB_r:
			case JSONLIB_f:
			case JSONLIB_a:
			case JSONLIB_l:
			case JSONLIB_s:
			case JSONLIB_n:

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
				JSONLIB_PushToken(&container, str[iter], iter);
				break;
		}

		JSONLIB_TokenPatternMatch(&container);
	}

	return container;
}

JSONLIB_TOKEN* CheckConsumeToken(JSONLIB_TOKENS* container, enum JSONLIB_TOKEN_TYPE type)
{
	if (container->tokens[container->processed].type == type)
	{
		return &container->tokens[container->processed++];
	}
	return NULL;
}

void JSONLIB_CleanupContainer(JSONLIB_TOKENS* container)
{
	JSONLIB_Deallocate(container->tokens);
	JSONLIB_Deallocate(container);
}

JSONptr JSONLIB_ParseObject(JSONLIB_TOKENS* container, const char* name);
JSONptr JSONLIB_ParseArray(JSONLIB_TOKENS* container, const char* name);

const char* JSONLIB_ParseString(JSONLIB_TOKENS* container)
{
	// Grab our starting token and validate we're parsing a string value
	const JSONLIB_TOKEN* strStart = CheckConsumeToken(container, JSONLIB_QUOTE);
	if (!strStart)
	{
		return NULL;
	}

	// Skip over all non-QUOTE tokens
	const JSONLIB_TOKEN* iter = &container->tokens[container->processed];
	while (iter->type != JSONLIB_QUOTE && container->processed < container->count)
	{
		container->processed++;
		iter = &container->tokens[container->processed];
	}

	// Grab our ending token and validate it matches what we're expecting
	const JSONLIB_TOKEN* strEnd = CheckConsumeToken(container, JSONLIB_QUOTE);
	if (!strEnd)
	{
		return NULL;
	}

	// Using the input positions copy the string data over to the JSONLIB heap
	// This string is NULL-terminated hence the additional char allocated
	const JSONLIB_size_t parsedStrLength = strEnd->inputPos - strStart->inputPos - 1;
	char* parsedStr = JSONLIB_Allocate(sizeof(char) * parsedStrLength + 1); 
	for (JSONLIB_size_t copyIter = 0; copyIter < parsedStrLength + 1; copyIter++)
	{
		parsedStr[copyIter] = container->inputStr[strStart->inputPos + copyIter + 1];
	}
	parsedStr[parsedStrLength] = '\0';

	return parsedStr;
}

enum JSONLIB_PARSEDNUMBERTYPE
{
	JSONLIB_PARSEDNUMBERTYPE_INTEGER,
	JSONLIB_PARSEDNUMBERTYPE_DECIMAL,
	JSONLIB_PARSEDNUMBERTYPE_ERROR
};

enum JSONLIB_PARSEDNUMBERTYPE JSONLIB_ParseNumber(JSONLIB_TOKENS* container)
{
	enum JSONLIB_PARSEDNUMBERTYPE typeToParse = JSONLIB_PARSEDNUMBERTYPE_INTEGER;
	JSONLIB_size_t lookaheadIter = container->processed;	
	while (lookaheadIter < container->count)
	{
		const enum JSONLIB_TOKEN_TYPE type = container->tokens[lookaheadIter].type;

		if (type == JSONLIB_DOT)
		{
			// If we've already picked up a decimal point this is bad data - BAIL
			if (typeToParse == JSONLIB_PARSEDNUMBERTYPE_DECIMAL)
			{
				typeToParse = JSONLIB_PARSEDNUMBERTYPE_ERROR;
				break;
			}
			typeToParse = JSONLIB_PARSEDNUMBERTYPE_DECIMAL;
		}

		if (type == JSONLIB_COMMA || type == JSONLIB_LEFT_BRACE || type == JSONLIB_RIGHT_BRACE)
		{
			break;
		}

		lookaheadIter++;
	}
	return typeToParse;
}

JSONLIB_int_t JSONLIB_ParseInteger(JSONLIB_TOKENS* container)
{
	JSONLIB_int_t num = 0;

	const enum JSONLIB_TOKEN_TYPE startTokenType = container->tokens[container->processed].type;
	if (startTokenType == JSONLIB_MINUS)
	{
		container->processed++;
	}

	while (container->processed < container->count)
	{
		const enum JSONLIB_TOKEN_TYPE type = container->tokens[container->processed].type;
		if (type == JSONLIB_COMMA || type == JSONLIB_RIGHT_BRACE || type == JSONLIB_RIGHT_SQUARE_BRACKET)
		{
			break;
		}

		if (type == JSONLIB_0 && num == 0)
		{
			continue;
		}

		num *= 10;

		switch (type)
		{
			default: break;
			case JSONLIB_0: break;
			case JSONLIB_1: num += 1; break;
			case JSONLIB_2: num += 2; break;
			case JSONLIB_3: num += 3; break;
			case JSONLIB_4: num += 4; break;
			case JSONLIB_5: num += 5; break;
			case JSONLIB_6: num += 6; break;
			case JSONLIB_7: num += 7; break;
			case JSONLIB_8: num += 8; break;
			case JSONLIB_9: num += 9; break;
		}

		container->processed++;
	}

	if (startTokenType == JSONLIB_MINUS)
	{
		num *= -1;
	}

	return num;
}

JSONLIB_float_t JSONLIB_ParseDecimal(JSONLIB_TOKENS* container)
{
	JSONLIB_float_t num = 0;

	const enum JSONLIB_TOKEN_TYPE startTokenType = container->tokens[container->processed].type;
	if (startTokenType == JSONLIB_MINUS)
	{
		container->processed++;
	}

	// Store the exponent and mantissa separately
	JSONLIB_int_t exponent = 0;
	JSONLIB_int_t mantissa = 0;

	// Multiplier used once we've extracted all of the data
	JSONLIB_float_t mantissaMultiplier = 1;

	// Store which component we're modifying in the loop
	JSONLIB_int_t* part = &exponent;
	while (container->processed < container->count)
	{
		const enum JSONLIB_TOKEN_TYPE type = container->tokens[container->processed].type;
		if (type == JSONLIB_COMMA || type == JSONLIB_RIGHT_BRACE || type == JSONLIB_RIGHT_SQUARE_BRACKET)
		{
			break;
		}

		// Switch to mantissa if we hit a decimal point
		if (type == JSONLIB_DOT)
		{
			container->processed++;
			part = &mantissa;
			continue;
		}

		*part *= 10;

		if (part == &mantissa)
		{
			mantissaMultiplier /= 10;
		}

		switch (type)
		{
			default: break;
			case JSONLIB_0: break;
			case JSONLIB_1: *part += 1; break;
			case JSONLIB_2: *part += 2; break;
			case JSONLIB_3: *part += 3; break;
			case JSONLIB_4: *part += 4; break;
			case JSONLIB_5: *part += 5; break;
			case JSONLIB_6: *part += 6; break;
			case JSONLIB_7: *part += 7; break;
			case JSONLIB_8: *part += 8; break;
			case JSONLIB_9: *part += 9; break;
		}

		container->processed++;
	}


	num = (f32)exponent;
	num += (f32)(mantissa * mantissaMultiplier);

	if (startTokenType == JSONLIB_MINUS)
	{
		num *= -1.f;
	}

	return num;
}

JSONptr JSONLIB_ParseValue(JSONLIB_TOKENS* container, const char* name)
{
	const enum JSONLIB_TOKEN_TYPE type = container->tokens[container->processed].type;

	switch (type)
	{
		case JSONLIB_LEFT_SQUARE_BRACKET: return JSONLIB_ParseArray(container, name);
		case JSONLIB_LEFT_BRACE: return JSONLIB_ParseObject(container, name);
		case JSONLIB_QUOTE:
		{
			const char* str = JSONLIB_ParseString(container);
			return JSONLIB_AllocateStringJSON(name, NULL, str);
		}
		case JSONLIB_TRUE:
		{
			container->processed++;
			return JSONLIB_AllocateJSON(name, NULL, JSONLIB_TRUE_TAG);
		}
		case JSONLIB_FALSE:
		{
			container->processed++;
			return JSONLIB_AllocateJSON(name, NULL, JSONLIB_FALSE_TAG);
		}
		case JSONLIB_NULL:
		{
			container->processed++;
			return JSONLIB_AllocateJSON(name, NULL, JSONLIB_NULL_TAG);
		}
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
		case JSONLIB_MINUS:
		{
			enum JSONLIB_PARSEDNUMBERTYPE numType = JSONLIB_ParseNumber(container);

			switch (numType)
			{
				case JSONLIB_PARSEDNUMBERTYPE_ERROR: return NULL;
				case JSONLIB_PARSEDNUMBERTYPE_INTEGER:
				{
					JSONLIB_int_t integer = JSONLIB_ParseInteger(container);
					return JSONLIB_AllocateIntegerJSON(name, NULL, integer);
				}
				case JSONLIB_PARSEDNUMBERTYPE_DECIMAL:
				{
					f32 fNum = JSONLIB_ParseDecimal(container);
					return JSONLIB_AllocateDecimalJSON(name, NULL, fNum);
				}
			}
			return NULL;
		}
		default: return NULL;
	}
}

JSONLIB_size_t JSONLIB_GetArrayLength(JSONLIB_TOKENS* container)
{
	JSONLIB_size_t arraySize = 0;
	JSONLIB_size_t tokenIter = container->processed;

	if (container->tokens[tokenIter++].type != JSONLIB_LEFT_SQUARE_BRACKET)
	{
		return arraySize;
	}

	while (tokenIter < container->count)
	{
		if (container->tokens[tokenIter].type == JSONLIB_COMMA)
		{
			arraySize++;
		}

		tokenIter++;

		if (container->tokens[tokenIter].type == JSONLIB_RIGHT_SQUARE_BRACKET)
		{
			arraySize++;
			break;
		}
	}

	return arraySize;
}

JSONptr JSONLIB_ParseArray(JSONLIB_TOKENS* container, const char* name)
{
	JSONLIB_size_t arraySize = JSONLIB_GetArrayLength(container);

	if (container->tokens[container->processed++].type != JSONLIB_LEFT_SQUARE_BRACKET)
	{
		return NULL;
	}

	// Array of pointers to parsed values
	JSONptr	arrayObj = JSONLIB_AllocateJSON(name, NULL, JSONLIB_ARRAY_TAG);
	arrayObj->values = JSONLIB_Allocate(sizeof(JSON*) * arraySize);
	arrayObj->valueCount = arraySize;

	JSONLIB_size_t arrayIter = 0;
	while (container->processed < container->count)
	{
		JSONptr parsedValue = JSONLIB_ParseValue(container, NULL);
		arrayObj->values[arrayIter++] = parsedValue;

		const enum JSONLIB_TOKEN_TYPE type = container->tokens[container->processed].type;
		if (type == JSONLIB_RIGHT_SQUARE_BRACKET)
		{
			break;
		}

		if (type == JSONLIB_COMMA)
		{
			container->processed++;
			continue;
		}
		
		return NULL;
	}

	if (container->tokens[container->processed++].type != JSONLIB_RIGHT_SQUARE_BRACKET)
	{
		return NULL;
	}

	return arrayObj;
}

JSONptr JSONLIB_ParseObject(JSONLIB_TOKENS* container, const char* name)
{
	// TODO: @Jon
	// Handle dellocation on parsing errors here?
	if (container->tokens[container->processed++].type != JSONLIB_LEFT_BRACE) 
	{
		return NULL;
	}

	JSONptr parsedObj = JSONLIB_AllocateJSON(name, NULL, JSONLIB_OBJECT_TAG);
	while (container->processed < container->count)
	{
		const enum JSONLIB_TOKEN_TYPE type = container->tokens[container->processed].type;
		if (type == JSONLIB_RIGHT_BRACE)
		{
			break;
		}

		if (type == JSONLIB_COMMA)
		{
			container->processed++;
		}

		const char* name = JSONLIB_ParseString(container);

		// TODO: @Jon
		// Handle dellocation on parsing errors here?
		if (container->tokens[container->processed++].type != JSONLIB_COLON)
		{
			JSONLIB_FreeJSON(parsedObj);
			return NULL; 
		}

		JSONptr parsedValue = JSONLIB_ParseValue(container, name);
		parsedValue->parent = parsedObj;

		// TODO: @Jon
		// Might make this a two-step process, figure out the number of values first and then allocate them once
		// Pretty sure this approach would lead to fragmentation
		const JSONLIB_size_t valueCount = parsedObj->valueCount + 1;
		JSON** newValues = JSONLIB_Allocate(sizeof(JSON) * valueCount);
		JSON** oldValues = parsedObj->values;

		for (JSONLIB_size_t copyIter = 0; copyIter < valueCount - 1; copyIter++)
		{
			newValues[copyIter] = oldValues[copyIter];
		}
		parsedObj->values = newValues;
		JSONLIB_Deallocate(oldValues);
		parsedObj->values[valueCount - 1] = parsedValue;
		parsedObj->valueCount = valueCount;
	}
	
	// TODO: @Jon
	// Handle dellocation on parsing errors here?
	if (container->tokens[container->processed++].type != JSONLIB_RIGHT_BRACE)
	{
		JSONLIB_FreeJSON(parsedObj);
		return NULL;
	}

	return parsedObj;
}

JSONptr JSONLIB_ParseJSON(const char *jsonString, const JSONLIB_size_t stringLength)
{
	JSONLIB_TOKENS container = JSONLIB_TokeniseString(jsonString, stringLength);
	const enum JSONLIB_TOKEN_TYPE type = container.tokens[container.processed].type;
	JSONptr root = NULL;

	switch (type)
	{
		default: break;
		case JSONLIB_LEFT_BRACE:
		{
			root = JSONLIB_ParseObject(&container, NULL);
			break;
		}
		case JSONLIB_LEFT_SQUARE_BRACKET:
		{
			root = JSONLIB_ParseArray(&container, NULL);
			break;
		}
	}
	return root;
}

const char *JSONLIB_MakeJSON(const JSONptr json, const u8 flags)
{
	return NULL;
}

void JSONLIB_FreeJSON(JSONptr json)
{
	for (JSONLIB_size_t valIter = 0; valIter < json->valueCount; valIter++)
	{
		JSONLIB_FreeJSON(json->values[valIter]);
	}

	json->valueCount = 0;
	JSONLIB_Deallocate(json->values);

	if (json->name != NULL)
	{
		JSONLIB_Deallocate((void*)json->name);
	}

	if (json->tags & JSONLIB_STRING_TAG)
	{
		JSONLIB_Deallocate((void*)json->string);
	}

	JSONLIB_Deallocate(json);
}

JSONptr JSONLIB_AllocateJSON(const char* name, JSONptr parent, const u8 tags)
{
	JSONptr allocJSON = JSONLIB_Allocate(sizeof(JSON));
	allocJSON->name = name;
	allocJSON->parent = parent;
	allocJSON->valueCount = 0;
	allocJSON->values = NULL;
	allocJSON->tags = tags;
	return allocJSON;
}

JSONptr JSONLIB_AllocateIntegerJSON(const char* name, JSONptr parent, const JSONLIB_int_t integer)
{
	JSONptr allocJSON = JSONLIB_AllocateJSON(name, parent, JSONLIB_INTEGER_TAG);
	allocJSON->integer = integer;
	return allocJSON;
}

JSONptr JSONLIB_AllocateStringJSON(const char* name, JSONptr parent, const char* string)
{
	JSONptr allocJSON = JSONLIB_AllocateJSON(name, parent, JSONLIB_STRING_TAG);
	allocJSON->string = string;
	return allocJSON;
}

JSONptr JSONLIB_AllocateDecimalJSON(const char* name, JSONptr parent, const JSONLIB_float_t decimal)
{
	JSONptr allocJSON = JSONLIB_AllocateJSON(name, parent, JSONLIB_DECIMAL_TAG);
	allocJSON->decimal = decimal;
	return allocJSON;
}

void JSONLIB_ClearJSON(const char *str)
{
	JSONLIB_Deallocate((void*)str);
}

#endif

#endif
