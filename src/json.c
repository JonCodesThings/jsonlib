#include <include/jsonlib/json.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

// TODO: @Jon
// Big TODO list for this file:
//  - Add convenience functions for checking if a JSON struct contains a value of given type
//  - Add proper tree searching (possible with some [] operator overloading? Opt-in C++ support would need to be added)
//	- Better commentary
//  - Way of enforcing precision of floating point string outputs
//  - Make C standard library dependencies optional (allow for user-provided alternatives to these functions)

#define JSON_DEFAULT_TOKENS 32

// NOTE: @Jon
// Tags for JSON nodes
const u8 JSON_STRING_TAG = 1 << 0;
const u8 JSON_INTEGER_TAG = 1 << 1;
const u8 JSON_DECIMAL_TAG = 1 << 2;
const u8 JSON_ARRAY_TAG = 1 << 3;
const u8 JSON_OBJECT_TAG = 1 << 4;
const u8 JSON_BOOLEAN_TAG = 1 << 5;
const u8 JSON_NULL_TAG = 1 << 6;

static bool HasTags(const JSON* json, const u8 tags)
{
	return json->tags & tags;
}

// NOTE: @Jon
// Some useful strings
static const char* const JSONtrueStr = "true";
static const char* const JSONfalseStr = "false";
static const char* const JSONnullStr = "null";

// NOTE: @Jon
// A struct for handling a char * string
typedef struct JSON_STRING_STRUCT
{
	char *raw;
	u32 length;
	u32 capacity;
} JSON_STRING_STRUCT;

// NOTE: @Jon
// JSON Token Types
// We're taking advantage of only supporting UTF-8 chars for some of the values
enum JSON_TOKEN_TYPE
{
	JSON_NULL = 0,

	VALUE,
	INTEGER,
	FLOAT,
	STRING,

	LEFT_BRACE = '{',
	RIGHT_BRACE = '}',
	LEFT_SQUARE_BRACKET = '[',
	RIGHT_SQUARE_BRACKET = ']',
	QUOTE = '"',
	COLON = ':',
	COMMA = ',',

	JSON_TRUE,
	JSON_FALSE,

	JSON_EOF,
	JSON_ERROR
};

// NOTE: @Jon
// Struct to store token information for the JSON
typedef struct JSON_TOKEN
{
	enum JSON_TOKEN_TYPE type; // = JSON_TOKEN_TYPE::JSON_NULL;
	char* str; // = NULL;
	u32 inputPos; // = 0;
} JSON_TOKEN;

typedef struct JSON_TOKENS
{
	JSON_TOKEN *tokens;
	u32 tokenCount;
	u32 tokenCapacity;
} JSON_TOKENS;

static JSON_ALLOC JSON_Allocate = malloc;
static JSON_DEALLOC JSON_Deallocate = free;

static inline void PushToken(JSON_TOKENS* container, enum JSON_TOKEN_TYPE tokenType, const u32 inputPosition)
{
	if (container->tokenCount + 1 >= container->tokenCapacity)
	{
		JSON_TOKEN* newTokenAlloc = (JSON_TOKEN*)JSON_Allocate(sizeof(JSON_TOKEN) * container->tokenCapacity * 2);
		assert(newTokenAlloc != NULL);

		memcpy(newTokenAlloc, container->tokens, sizeof(JSON_TOKEN) * container->tokenCapacity);
		JSON_Deallocate(container->tokens);

		container->tokens = newTokenAlloc;
		container->tokenCapacity *= 2;
	}

	JSON_TOKEN* currentToken = &container->tokens[container->tokenCount++];
	currentToken->type = tokenType;
	currentToken->inputPos = inputPosition;
}

static inline JSON_TOKEN* GetLastToken(const JSON_TOKENS* container)
{
	if (container->tokenCount == 0) return NULL;

	return &container->tokens[container->tokenCount - 1];
}

static inline void CopyAllocateSubString(JSON_TOKEN* token, const char *value, u32 valueLength)
{
	token->str = (char*)JSON_Allocate(sizeof(char) * valueLength + 1);
	token->str[valueLength] = '\0';
	memcpy((void*)token->str, value, sizeof(char) * valueLength);
}

// NOTE: @Jon
// Parses a value
static void PushStringToken(JSON_TOKENS* container, const u32 inputPosition, const char *value, u32 valueLength)
{
	PushToken(container, STRING, inputPosition);
	JSON_TOKEN* token = GetLastToken(container);
	CopyAllocateSubString(token, value, valueLength);
}

static void PushValueToken(JSON_TOKENS* container, const u32 inputPosition, const char *value, u32 valueLength)
{
	if (valueLength == 0) return;

	PushToken(container, VALUE, inputPosition);
	JSON_TOKEN* token = GetLastToken(container);
	CopyAllocateSubString(token, value, valueLength);
}

static void ParseValue(JSON_TOKEN *token, const char *value, u32 valueLength)
{
	token->type = JSON_ERROR;


	// If it's a boolean value
	if ((value[0] == 't' && value[3] == 'e') || (value[0] == 'f'&& value[4] == 'e'))
	{
		if (value[0] == 't')
			token->type = JSON_TRUE;
		else
			token->type = JSON_FALSE;
		return;
	}
	// If it's just null
	if (value[0] == 'n' && value[valueLength] == 'l')
	{
		token->type = JSON_NULL;
	}
}

// NOTE: @Jon
// Allocates a JSON node
// Uses the allocation functions specified with JSONLIB_SetAllocator
JSON* JSONLIB_AllocateJSON(const char* name, struct JSON* parent)
{
	JSON* node = JSON_Allocate(sizeof(JSON));
	node->name = name;
	node->parent = NULL;
	node->values = NULL;
	node->valueCount = 0;
	node->tags = JSON_OBJECT_TAG;
	
	if (parent != NULL)
	{
		JSONLIB_AddValueJSON(parent, node);
	}
	
	return node;
}

// NOTE: @Jon
// Allocates a JSON node
// Uses the allocation functions specified with JSONLIB_SetAllocator
JSON* JSONLIB_AllocateIntegerJSON(const char* name, struct JSON* parent, const i32 integer)
{
	JSON* node = JSONLIB_AllocateJSON(name, parent);
	node->integer = integer;
	node->tags = JSON_INTEGER_TAG;
	return node;
}

// NOTE: @Jon
// Allocates a JSON node
// Uses the allocation functions specified with JSONLIB_SetAllocator
JSON* JSONLIB_AllocateDecimalJSON(const char* name, struct JSON* parent, const f32 decimal)
{
	JSON* node = JSONLIB_AllocateJSON(name, parent);
	node->decimal = decimal;
	node->tags = JSON_DECIMAL_TAG;
	return node;
}

// NOTE: @Jon
// Allocates a JSON node
// Uses the allocation functions specified with JSONLIB_SetAllocator
JSON* JSONLIB_AllocateStringJSON(const char* name, struct JSON* parent, const char* string)
{
	JSON* node = JSONLIB_AllocateJSON(name, parent);
	node->string = string;
	node->tags = JSON_STRING_TAG;
	return node;
}

// NOTE: @Jon
// Allocates a JSON node
// Uses the allocation functions specified with JSONLIB_SetAllocator
JSON* JSONLIB_AllocateBooleanJSON(const char* name, struct JSON* parent, const bool boolean)
{
	JSON* node = JSONLIB_AllocateJSON(name, parent);
	node->boolean = boolean;
	node->tags = JSON_BOOLEAN_TAG;
	return node;
}

// NOTE: @Jon
// Adds a value to the JSON node given
void JSONLIB_AddValueJSON(JSON *json, JSON *val)
{
	json->valueCount++;

	if (val != NULL)
		val->parent = json;

	// Allocate the memory
	JSON **newValueArray = (JSON**)JSON_Allocate(sizeof(JSON*) * json->valueCount);
	assert(newValueArray != NULL);

	// Copy over the old array and free the memory
	if (json->valueCount - 1 > 0)
		memcpy(newValueArray, json->values, sizeof(JSON*) * (json->valueCount - 1));
	if (json->values != NULL)
		JSON_Deallocate((void*)json->values);

	json->values = newValueArray;

	if (val != NULL)
		json->values[json->valueCount - 1] = val;
}

// NOTE: @Jon
// Convenience function for adding a new value to an array that's being parsed
static void AddValueToArray(JSON** json)
{
	JSONLIB_AddValueJSON((*json), NULL);
	JSON* newVal = (JSON*)JSON_Allocate(sizeof(JSON));
	newVal->parent = (*json);
	assert((*json)->values != NULL);
	(*json)->values[(*json)->valueCount - 1] = newVal;
	(*json) = newVal;
	(*json)->name = NULL;
}

// NOTE: @Jon
// Convenience function for adding a decimal number to a node
static void AddDecimalValue(JSON **json, const char *decimalStr)
{
	(*json)->decimal = (f32)atof(decimalStr);
	(*json)->tags = 0;
	(*json)->tags |= JSON_DECIMAL_TAG;
	(*json)->valueCount = 0;
	*json = (*json)->parent;
	JSON_Deallocate((void*)decimalStr);
}

// NOTE: @Jon
// Convenience function for adding an integer number to a node
static void AddIntegerValue(JSON **json, const char *integerStr)
{
	(*json)->integer = atoi(integerStr);
	(*json)->tags = 0;
	(*json)->tags |= JSON_INTEGER_TAG;
	(*json)->valueCount = 0;
	*json = (*json)->parent;
	JSON_Deallocate((void*)integerStr);
}

// NOTE: @Jon
// Convenience function for adding a string to a node
static void AddStringValue(JSON **json, const char *str)
{
	(*json)->string = str;
	(*json)->tags = 0;
	(*json)->tags |= JSON_STRING_TAG;
	(*json)->valueCount = 0;
	*json = (*json)->parent;
}

// NOTE: @Jon
// Convenience function for adding a boolean value to a node
static void AddBooleanValue(JSON** json, bool boolean)
{
	(*json)->boolean = boolean;
	(*json)->tags = 0;
	(*json)->tags |= JSON_BOOLEAN_TAG;
	(*json)->valueCount = 0;
	*json = (*json)->parent;
}

// NOTE: @Jon
// Convenience function for adding a null value to a node
static void AddNullValue(JSON** json)
{
	(*json)->values = NULL;
	(*json)->tags = 0;
	(*json)->tags |= JSON_NULL_TAG;
	(*json)->valueCount = 0;
	*json = (*json)->parent;
}

// NOTE: @Jon
// Function for tokenising the given input string
static JSON_TOKENS* Tokenise(const char* jsonString, u32 stringLength)
{
	JSON_TOKENS* container = (JSON_TOKENS*)JSON_Allocate(sizeof(JSON_TOKENS));
	container->tokens = (JSON_TOKEN*)JSON_Allocate(sizeof(JSON_TOKEN) * JSON_DEFAULT_TOKENS);
	container->tokenCount = 0;
	container->tokenCapacity = JSON_DEFAULT_TOKENS;

	for (u32 i = 0; i < stringLength; ++i)
	{
		JSON_TOKEN* prevToken = GetLastToken(container);

		switch (jsonString[i])
		{
			case LEFT_BRACE:
			case LEFT_SQUARE_BRACKET:
				PushToken(container, jsonString[i], i);
				continue;
			case RIGHT_BRACE:
			case RIGHT_SQUARE_BRACKET:
			case COMMA:
				if (prevToken != NULL)
				{
					u32 tokenStart = prevToken->inputPos + 1;
					u32 tokenEnd = i;

					PushValueToken(container, tokenStart, &jsonString[tokenStart], tokenEnd - tokenStart);
				}
				PushToken(container, jsonString[i], i);
				continue;
			case QUOTE:
				{
					if (prevToken != NULL && prevToken->type == QUOTE)
					{
						u32 stringStart = prevToken->inputPos + 1;
						u32 stringEnd = i;
						PushStringToken(container, stringStart, &jsonString[stringStart], stringEnd - stringStart);
					}

					PushToken(container, QUOTE, i);
				}
				continue;
			case COLON:
				PushToken(container, COLON, i);
				continue;
			default:
				break;
		}

		if (prevToken == NULL) continue;

		if (jsonString[i] == ' ') continue;
	}
	return container;
}

// NOTE: @Jon
// Internal parsing function
static JSON *ParseJSONInternal(JSON_TOKEN *tokens, u32 tokenCount, const u8 tags, JSON *parent)
{
	JSON* json = NULL;
	if ((!(tags & JSON_ARRAY_TAG) && !(tags & JSON_OBJECT_TAG)) || parent == NULL)
	{
		json = (JSON*)JSON_Allocate(sizeof(JSON));

		assert(json != NULL);

		json->name = NULL;
		json->valueCount = 0;
		json->tags = 0;
		json->tags |= tags;
		json->values = NULL;
		json->parent = parent;
	}
	else
		json = parent;

	assert(json != NULL);

	bool finished = true;
	for (u32 i = 1; i < tokenCount - 1; ++i)
	{
		switch (tokens[i].type)
		{
		case LEFT_BRACE:
		{
			assert(json != NULL);

			if (HasTags(json, JSON_ARRAY_TAG))
			{
				JSONLIB_AddValueJSON(json, NULL);
				JSON* newVal = (JSON*)JSON_Allocate(sizeof(JSON));
				newVal->parent = json;
				assert(json->values != NULL);
				json->values[json->valueCount - 1] = newVal;
				newVal->name = NULL;
				newVal->valueCount = 0;
				json = newVal;
			}

			// Go one layer deeper to parse an object
			ParseJSONInternal(&tokens[i], i, JSON_OBJECT_TAG, json);
			json->tags = 0; json->tags |= JSON_OBJECT_TAG;
			json = json->parent;
			break;
		}
		case LEFT_SQUARE_BRACKET:
		{
			assert(json != NULL);

			json->tags = 0; json->tags |= JSON_ARRAY_TAG;
			// Go one layer deeper to parse an array
			ParseJSONInternal(&tokens[i], i, JSON_ARRAY_TAG, json);
			json = json->parent;
			break;
		}
		case RIGHT_BRACE:
			break;
		case VALUE:
		{
			// Allocate a node for this identifier
			JSON *val = (JSON*)JSON_Allocate(sizeof(JSON));
			JSONLIB_AddValueJSON (json, val);
			assert(json->values != NULL);
			json = val;

			json->name = tokens[i].str;
			json->values = NULL;
			json->valueCount = 0;
			json->tags = 0;
			break;
		}
		case COLON:
			// If we see a colon the next token *should* be a value
			break;
		case STRING:
		{
			if (HasTags(json, JSON_ARRAY_TAG))
				AddValueToArray(&json);
			// Add a string value to the node
			AddStringValue(&json, tokens[i].str);
			break;
		}
		case INTEGER:
		{
			if (HasTags(json, JSON_ARRAY_TAG))
				AddValueToArray(&json);
			// Add an integer value to the node
			AddIntegerValue(&json, tokens[i].str);
			break;
		}
		case FLOAT:
		{
			if (HasTags(json, JSON_ARRAY_TAG))
				AddValueToArray(&json);
			// Add a float value to the node
			AddDecimalValue(&json, tokens[i].str);
			break;
		}
		case JSON_TRUE:
		case JSON_FALSE:
		{
			if (HasTags(json, JSON_ARRAY_TAG))
				AddValueToArray(&json);
			// Add a boolean value to the node
			AddBooleanValue(&json, tokens[i].type == JSON_TRUE);
			break;
		}
		case JSON_NULL:
		{
			if (HasTags(json, JSON_ARRAY_TAG))
				AddValueToArray(&json);
			// Add a null value to the node
			AddNullValue(&json);
			break;
		}
		default: break;
		}
	}

	if (finished)
		return json;

	JSONLIB_FreeJSON(json);
	return NULL;
}

// NOTE: @Jon
// Sets the allocation functions for the library to use internally
void JSONLIB_SetAllocator(JSON_ALLOC alloc, JSON_DEALLOC dealloc)
{
	JSON_Allocate = alloc;
	JSON_Deallocate = dealloc;
}

// NOTE: @Jon
// Convenience function for freeing memory
static void FreeTokenMemory(JSON_TOKENS *tokens)
{
	for (u32 i = 0; i < tokens->tokenCount; ++i)
	{
		if (tokens->tokens[i].str != NULL)
			JSON_Deallocate(tokens->tokens[i].str);
	}
	JSON_Deallocate(tokens->tokens);
	JSON_Deallocate(tokens);
}

// NOTE: @Jon
// Parses a JSON string
JSON *JSONLIB_ParseJSON(const char *jsonString, u32 stringLength)
{
	JSON_TOKENS *tokens = Tokenise(jsonString, stringLength);

	JSON *json = ParseJSONInternal(tokens->tokens, tokens->tokenCount, JSON_OBJECT_TAG, NULL);

	FreeTokenMemory(tokens);

	if (json == NULL)
	{
		return NULL;
	}

	return json;
}

static const char *DecimalValueToString(char *dest, const f32 decimal, const u32 stringSize)
{
	snprintf(dest, sizeof(char) * stringSize, "%f", decimal);
	return dest;
}

static const char *IntegerValueToString(char *dest, const i32 integer, const u32 stringSize)
{
	snprintf(dest, sizeof(char) * stringSize, "%d", integer);
	return dest;
}

static const char* BooleanValueToString(char* dest, const bool boolean)
{
	const char* copy = boolean ? JSONtrueStr : JSONfalseStr;
	u32 len = boolean ? 5 : 6;
	memcpy(dest, copy, sizeof(char) * len);
	return dest;
}

static const char* NullValueToString(char* dest)
{
	memcpy(dest, JSONnullStr, sizeof(char) * 5);
	return dest;
}

static char *MakeStringValueString(const char *str, const u32 strLen)
{
	char *memberNameString = (char*)JSON_Allocate(sizeof(char) * strLen + 3);
	memberNameString[0] = memberNameString[strLen + 1] = '\"';
	memberNameString[strLen + 2] = '\0';
	memcpy(&memberNameString[1], str, sizeof(char) * strLen);
	return memberNameString;
}

static char *MakeValueString(const JSON *json, const u32 stringSize)
{
	char *valueString = NULL;

	if (HasTags(json, JSON_DECIMAL_TAG | JSON_INTEGER_TAG | JSON_BOOLEAN_TAG | JSON_NULL_TAG))
		valueString = (char*)JSON_Allocate(sizeof(char) * stringSize);

	if (HasTags(json, JSON_DECIMAL_TAG))
		DecimalValueToString(valueString, json->decimal, stringSize);
	else if (HasTags(json, JSON_INTEGER_TAG))
		IntegerValueToString(valueString, json->integer, stringSize);
	else if (HasTags(json, JSON_BOOLEAN_TAG))
		BooleanValueToString(valueString, json->boolean);
	else if (HasTags(json, JSON_NULL_TAG))
		NullValueToString(valueString);
	else if (HasTags(json, JSON_STRING_TAG))
	{
		const u32 valueStrLen = (u32)strlen(json->string);
		valueString = MakeStringValueString(json->string, valueStrLen);
	}
	
	return valueString;
}

static bool StringCapacityHelper(JSON_STRING_STRUCT *str, u32 potentialAllocation)
{
	if (str->length >= str->capacity || str->length + potentialAllocation >= str->capacity)
	{
		char *doubled = (char*)JSON_Allocate(sizeof(char) * str->capacity * 2);
		if (doubled == NULL)
			return false;
		memcpy(doubled, str->raw, sizeof(char) * str->length);
		JSON_Deallocate(str->raw);
		str->raw = doubled;
		str->capacity *= 2;
	}
	return true;
}

static bool AppendCharToString(JSON_STRING_STRUCT *str, const char alloc)
{
	if (!StringCapacityHelper(str, 1))
		return false;
	str->raw[str->length++] = alloc;
	return true;
}

static bool AppendStringToString(JSON_STRING_STRUCT *str, const char* chars, const u32 charsLen)
{
	if (!StringCapacityHelper(str, charsLen))
		return false;
	memcpy(&str->raw[str->length], chars, sizeof(char) * charsLen);
	str->length += charsLen;
	return true;
}

static JSON_STRING_STRUCT *MakeJSONPrettyNewline(JSON_STRING_STRUCT *str)
{
	StringCapacityHelper(str, 1);
	str->raw[str->length++] = '\n';
	return str;
}

static JSON_STRING_STRUCT *MakeJSONInternal(JSON_STRING_STRUCT *str, const JSON *const json, const bool humanReadable)
{
	if (json->name != NULL)
	{
		const u32 nameLen = (u32)strlen(json->name);
		if (nameLen > 0)
		{
			char* name = MakeStringValueString(json->name, nameLen);
			u32 valueStrLen = (u32)strlen(name);
			AppendStringToString(str, name, valueStrLen);
			AppendCharToString(str, ':');
			JSON_Deallocate(name);
		}
	}

	if (HasTags(json, JSON_OBJECT_TAG))
	{
		AppendCharToString(str, '{');
	}
	else if (HasTags(json, JSON_ARRAY_TAG))
	{
		AppendCharToString(str, '[');
	}

	if (json->valueCount > 0)
	{
		for (u32 i = 0; i < json->valueCount; ++i)
		{
			MakeJSONInternal(str, json->values[i], humanReadable);

			if (i != json->valueCount - 1)
			{
				AppendCharToString(str, ',');
			}

			if (humanReadable)
				MakeJSONPrettyNewline(str);
		}
	}
	else
	{
		// TODO: @Jon
		// Shouldn't hardcode the value string size!
		char* valString = MakeValueString(json, 64);

		if (valString != NULL)
		{
			const u32 valStrLen = (u32)strlen(valString);
			AppendStringToString(str, valString, valStrLen);
			JSON_Deallocate(valString);
		}
	}

	if (HasTags(json, JSON_OBJECT_TAG))
	{
		AppendCharToString(str, '}');
	}
	else if (HasTags(json, JSON_ARRAY_TAG))
	{
		AppendCharToString(str, ']');
	}

	return str;
}

// NOTE: @Jon
// Makes a JSON string from a given tree input
const char * JSONLIB_MakeJSON(const JSON * const json, const bool humanReadable)
{
	JSON_STRING_STRUCT jsonString;
	jsonString.capacity = 64;
	jsonString.length = 0;
	jsonString.raw = (char*)JSON_Allocate(sizeof(char) * 1024);

	MakeJSONInternal(&jsonString, json, humanReadable);

	AppendCharToString(&jsonString, '\0');

	return jsonString.raw;
}

// NOTE: @Jon
// Gets a node from a given tree
JSON *JSONLIB_GetValueJSON(const char *name, u32 nameLength, JSON *json)
{
	assert(json != NULL);
	for (u32 i = 0; i < json->valueCount; ++i)
	{
		if (strcmp((*json->values[i]).name, name) == 0)
			return json->values[i];
	}
	return NULL;
}

// NOTE: @Jon
// Frees memory related to a given tree
void JSONLIB_FreeJSON(JSON *json)
{
	if (json == NULL)
		return;

	if (json->parent != NULL)
	{
		for (u32 i = 0; i < json->parent->valueCount; ++i)
		{
			if (json->parent->values[i] == json)
			{
				json->parent->values[i] = NULL;
				break;
			}
		}
	}

	for (u32 i = 0; i < json->valueCount; ++i)
	{
		JSONLIB_FreeJSON(json->values[i]);
	}

	if (json->name)
		JSON_Deallocate((void*)json->name);

	if (HasTags(json, JSON_STRING_TAG))
		JSON_Deallocate((void*)json->string);

	if (json->valueCount > 0)
		JSON_Deallocate(json->values);

	JSON_Deallocate(json);
}

void JSONLIB_ClearJSON(const char *str)
{
	JSON_Deallocate((void*)str);
	str = NULL;
}
