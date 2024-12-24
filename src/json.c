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
#define JSON_DEFAULT_DIVIDER_STACK_SIZE 4

// NOTE: @Jon
// Tags for JSON nodes
const u8 JSON_STRING_TAG = 1 << 0;
const u8 JSON_INTEGER_TAG = 1 << 1;
const u8 JSON_DECIMAL_TAG = 1 << 2;
const u8 JSON_ARRAY_TAG = 1 << 3;
const u8 JSON_OBJECT_TAG = 1 << 4;
const u8 JSON_BOOLEAN_TAG = 1 << 5;
const u8 JSON_NULL_TAG = 1 << 6;

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
// JSON Divider Stack for tracking different dividers (e.g. {} or [])
typedef struct JSON_DIVIDER_STACK
{
	char *dividerStack;
	u32 dividerCount;
	u32 dividerCapacity;
} JSON_DIVIDER_STACK;

// NOTE: @Jon
// JSON Token Types
enum JSON_TOKEN_TYPE
{
	LEFT_BRACE,
	RIGHT_BRACE,
	LEFT_SQUARE_BRACKET,
	RIGHT_SQUARE_BRACKET,
	COLON,
	IDENTIFIER,
	COMMA,
	INTEGER,
	FLOAT,
	STRING,
	JSON_TRUE,
	JSON_FALSE,
	JSON_NULL,
	JSON_EOF,
	JSON_ERROR,
	JSON_TOKEN_TYPE_COUNT
};

// NOTE: @Jon
// Struct to store token information for the JSON
typedef struct JSON_TOKEN
{
	enum JSON_TOKEN_TYPE type; // = JSON_TOKEN_TYPE::JSON_ERROR;
	char* identifier; // = NULL;
} JSON_TOKEN;

typedef struct JSON_TOKENS
{
	JSON_TOKEN *tokens;
	u32 tokenCount;
	u32 tokenCapacity;
} JSON_TOKENS;

static JSON_ALLOC JSON_Allocate = malloc;
static JSON_DEALLOC JSON_Deallocate = free;

// NOTE: @Jon
// Parses an identifier
static void ParseIdentifier(JSON_TOKEN *token, const char *ident, u32 valueLength)
{
	token->type = IDENTIFIER;
	token->identifier = (char*)JSON_Allocate(sizeof(char) * ((size_t)valueLength + 1));
	token->identifier[valueLength] = '\0';
	memcpy((void*)token->identifier, ident, sizeof(char) * (valueLength));
}

// NOTE: @Jon
// Parses a value
static void ParseValue(JSON_TOKEN *token, const char *value, u32 valueLength)
{
	// If the value matches a string
	if (value[0] == '"' && value[valueLength] == '"')
	{
		token->type = STRING;
		token->identifier = (char*)JSON_Allocate(sizeof(char) * (valueLength));
		token->identifier[valueLength - 1] = '\0';
		memcpy(token->identifier, &value[1], sizeof(char) * (valueLength - 1));
	}
	// If it starts with a number
	else if ((value[0] > 47 && value[0] < 58) || value[0] == '-')
	{
		token->identifier = (char*)JSON_Allocate(sizeof(char) * ((size_t)valueLength + 1));
		token->identifier[valueLength] = '\0';
		memcpy(token->identifier, value, sizeof(char) * (valueLength));
		for (u32 i = 0; i < valueLength; ++i)
		{
			if (value[i] == '.')
			{
				token->type = FLOAT;
				return;
			}
		}
		token->type = INTEGER;
	}
	// If it's a boolean value
	else if ((value[0] == 't' && (value[3] == 'e')) || (value[0] == 'f'&& (value[4] == 'e')))
	{
		if (value[0] == 't')
			token->type = JSON_TRUE;
		else
			token->type = JSON_FALSE;
	}
	// If it's just null
	else if (value[0] == 'n' && value[valueLength] == 'l')
		token->type = JSON_NULL;
	// Otherwise give up
	else
		token->type = JSON_ERROR;
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

static void DividerStackPush(JSON_DIVIDER_STACK *const stack, const char toPush)
{
	if (stack->dividerCount + 1 >= stack->dividerCapacity)
	{
		char *expanded = (char*)JSON_Allocate(sizeof(char) * stack->dividerCapacity * 2);

		assert(expanded != NULL);

		memcpy(expanded, stack->dividerStack, sizeof(char) * stack->dividerCount);
		stack->dividerCapacity *= 2;
		JSON_Deallocate(stack->dividerStack);
		stack->dividerStack = expanded;
	}
	stack->dividerStack[stack->dividerCount++] = toPush;
}

static char DividerStackPop(JSON_DIVIDER_STACK *const stack)
{
	return stack->dividerStack[--stack->dividerCount];
}

// NOTE: @Jon
// Get the closing token to the current divider
static u32 GetCloserOffset(JSON_TOKEN *token, u32 startToken, u32 tokenCount, JSON_DIVIDER_STACK *stack, enum JSON_TOKEN_TYPE opener, enum JSON_TOKEN_TYPE closer)
{
	for (u32 i = 0; i < tokenCount; ++i)
	{
		if (token[i].type == LEFT_BRACE || token[i].type == LEFT_SQUARE_BRACKET)
			DividerStackPush(stack, 'S');
		else if (token[i].type == RIGHT_BRACE || token[i].type == RIGHT_SQUARE_BRACKET)
			DividerStackPop(stack);

		if (stack->dividerCount == 0)
			return i - startToken;
	}
	return -1;
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
static JSON_TOKENS* Tokenise(const char* jsonString, u32 stringLength, JSON_DIVIDER_STACK* dividerStack)
{
	JSON_TOKENS* container = (JSON_TOKENS*)JSON_Allocate(sizeof(JSON_TOKENS));
	container->tokens = (JSON_TOKEN*)JSON_Allocate(sizeof(JSON_TOKEN) * JSON_DEFAULT_TOKENS);
	container->tokenCount = 0;
	container->tokenCapacity = JSON_DEFAULT_TOKENS;

	for (u32 i = 0; i < stringLength; ++i)
	{
		if (container->tokenCount >= container->tokenCapacity)
		{
			JSON_TOKEN* newTokenAlloc = (JSON_TOKEN*)JSON_Allocate(sizeof(JSON_TOKEN) * container->tokenCapacity * 2);
			assert(newTokenAlloc != NULL);

			memcpy(newTokenAlloc, container->tokens, sizeof(JSON_TOKEN) * container->tokenCapacity);
			JSON_Deallocate(container->tokens);

			container->tokens = newTokenAlloc;
			container->tokenCapacity *= 2;

		}
		switch (jsonString[i])
		{
			// NOTE: @Jon
			// Fairly standard JSON character handling
		case '{':
			container->tokens[container->tokenCount++].type = LEFT_BRACE;
			DividerStackPush(dividerStack, jsonString[i]);
			break;
		case '}':
			container->tokens[container->tokenCount++].type = RIGHT_BRACE;
			DividerStackPop(dividerStack);
			break;
		case '[':
			container->tokens[container->tokenCount++].type = LEFT_SQUARE_BRACKET;
			DividerStackPush(dividerStack, jsonString[i]);
			break;
		case ']':
			container->tokens[container->tokenCount++].type = RIGHT_SQUARE_BRACKET;
			DividerStackPop(dividerStack);
			break;
		case '"':
			if (dividerStack->dividerStack[dividerStack->dividerCount - 1] == '"')
				dividerStack->dividerCount--;
			else
				DividerStackPush(dividerStack, jsonString[i]);
			break;
		case ':':
			container->tokens[container->tokenCount++].type = COLON;
			break;
		case ',':
			container->tokens[container->tokenCount++].type = COMMA;
			break;
		default:
			if (container->tokens[container->tokenCount - 1].type == COLON)
			{
				if (dividerStack->dividerStack[dividerStack->dividerCount - 1] == '"')
				{
					for (u32 iter = i; iter < stringLength; ++iter)
					{
						if (jsonString[iter] == '"')
						{
							// Get the length of the string
							u32 length = (iter - i) + 1;

							ParseValue(&container->tokens[container->tokenCount++], &jsonString[i - 1], length);

							// Stop checking for an identifier
							i = iter - 1;
							break;
						}
					}
				}
				else if ((jsonString[i] > 47 && jsonString[i] < 58) || jsonString[i] == '-')
				{
					for (u32 iter = i; iter < stringLength; ++iter)
					{
						if (jsonString[iter] == ',' || jsonString[iter] == '}')
						{
							u32 length = iter - i;
							ParseValue(&container->tokens[container->tokenCount++], &jsonString[i], length);
							i = iter - 1;
							break;
						}
					}
				}
				else if (jsonString[i] == 'n' || jsonString[i] == 't' || jsonString[i] == 'f')
				{
					for (u32 iter = i; iter < stringLength; ++iter)
					{
						if (jsonString[iter] == ',' || jsonString[iter] == '}')
						{
							u32 length = iter - i;
							ParseValue(&container->tokens[container->tokenCount++], &jsonString[i], length - 1);
							i = iter - 1;
							break;
						}
					}
				}
			}
			else if ((container->tokens[container->tokenCount - 1].type == COMMA || container->tokens[container->tokenCount - 1].type == LEFT_BRACE || container->tokens[container->tokenCount - 1].type == LEFT_SQUARE_BRACKET))
			{
				if (dividerStack->dividerStack[dividerStack->dividerCount - 1] == '"')
				{
					for (u32 iter = i; iter < stringLength; ++iter)
					{
						if (jsonString[iter] == '"')
						{
							// Get the length of the string
							u32 length = iter - i;

							ParseIdentifier(&container->tokens[container->tokenCount++] , &jsonString[i], length);
							// printf("%s\n", container->tokens[container->tokenCount].identifier);

							// Stop checking for an identifier
							i = iter - 1;
							break;
						}
					}
				}
				bool divstack = false;
				if (dividerStack->dividerCount > 1)
					divstack = dividerStack->dividerStack[dividerStack->dividerCount - 2] == '[';
				if (dividerStack->dividerStack[dividerStack->dividerCount - 1] == '[' || divstack)
				{
					if (jsonString[i] > 47 && jsonString[i] < 58)
					{
						for (u32 iter = i; iter < stringLength; ++iter)
						{
							if (jsonString[iter] == ',' || jsonString[iter] == '}' || jsonString[iter] == ']' || jsonString[iter] == '\r' || jsonString[iter] == '\n')
							{
								u32 length = iter - i;
								ParseValue(&container->tokens[container->tokenCount++], &jsonString[i], length);
								i = iter - 1;
								break;
							}
						}
					}
				}
			}
			break;
		}
	}
	return container;
}

// NOTE: @Jon
// Corrects some problems with the tokenisation process after it has finished running
static bool CorrectTokens(JSON_TOKENS* tokens, JSON_DIVIDER_STACK* dividerStack)
{
	for (u32 i = 0; i < dividerStack->dividerCapacity; ++i)
	{
		dividerStack->dividerStack[i] = ' ';
	}

	for (u32 i = 0; i < tokens->tokenCount; ++i)
	{
		JSON_TOKEN* prev = i > 0 ? &tokens->tokens[i - 1] : NULL;
		JSON_TOKEN* next = i < tokens->tokenCount - 1 ? &tokens->tokens[i + 1] : NULL;
		JSON_TOKEN* current = &tokens->tokens[i];

		switch (current->type)
		{
		default:
			break;
		case LEFT_BRACE:
			DividerStackPush(dividerStack, '{');
			break;
		case LEFT_SQUARE_BRACKET:
			DividerStackPush(dividerStack, '[');
			break;
		case RIGHT_BRACE:
		case RIGHT_SQUARE_BRACKET:
			DividerStackPop(dividerStack);
			break;
		case JSON_ERROR:
			return false;
		}

		if (current->type == IDENTIFIER)
		{
			if (dividerStack->dividerCount != 0 && next != NULL)
			{
				if (dividerStack->dividerStack[dividerStack->dividerCount - 1] == '[' && next->type != COLON)
					current->type = STRING;
			}
		}
	}

	return true;
}


// NOTE: @Jon
// Internal parsing function
static JSON *ParseJSONInternal(JSON_TOKEN *tokens, u32 tokenCount, JSON_DIVIDER_STACK *stack, const u8 tags, JSON *parent)
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
			u32 offset = GetCloserOffset(&tokens[i], 0, tokenCount - i, stack, LEFT_BRACE, RIGHT_BRACE);

			if (offset == -1)
			{
				finished = false;
				break;
			}

			assert(json != NULL);

			if (json->tags & JSON_ARRAY_TAG)
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
			ParseJSONInternal(&tokens[i], offset + 1, stack, JSON_OBJECT_TAG, json);
			i += offset;
			json->tags = 0; json->tags |= JSON_OBJECT_TAG;
			json = json->parent;
			break;
		}
		case LEFT_SQUARE_BRACKET:
		{
			u32 offset = GetCloserOffset(&tokens[i], 0, tokenCount - i, stack, LEFT_SQUARE_BRACKET, RIGHT_SQUARE_BRACKET);

			if (offset == -1)
			{
				finished = false;
				break;
			}
			assert(json != NULL);

			json->tags = 0; json->tags |= JSON_ARRAY_TAG;
			// Go one layer deeper to parse an array
			ParseJSONInternal(&tokens[i], offset + 1, stack, JSON_ARRAY_TAG, json);
			i += offset;
			json = json->parent;
			break;
		}
		case RIGHT_BRACE:
			break;
		case IDENTIFIER:
		{
			// Allocate a node for this identifier
			JSON *val = (JSON*)JSON_Allocate(sizeof(JSON));
			JSONLIB_AddValueJSON (json, val);
			assert(json->values != NULL);
			json = val;

			json->name = tokens[i].identifier;
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
			if (json->tags & JSON_ARRAY_TAG)
				AddValueToArray(&json);
			// Add a string value to the node
			AddStringValue(&json, tokens[i].identifier);
			break;
		}
		case INTEGER:
		{
			if (json->tags & JSON_ARRAY_TAG)
				AddValueToArray(&json);
			// Add an integer value to the node
			AddIntegerValue(&json, tokens[i].identifier);
			break;
		}
		case FLOAT:
		{
			if (json->tags & JSON_ARRAY_TAG)
				AddValueToArray(&json);
			// Add a float value to the node
			AddDecimalValue(&json, tokens[i].identifier);
			break;
		}
		case JSON_TRUE:
		case JSON_FALSE:
		{
			if (json->tags & JSON_ARRAY_TAG)
				AddValueToArray(&json);
			// Add a boolean value to the node
			AddBooleanValue(&json, tokens[i].type == JSON_TRUE);
			break;
		}
		case JSON_NULL:
		{
			if (json->tags & JSON_ARRAY_TAG)
				AddValueToArray(&json);
			// Add a null value to the node
			AddNullValue(&json);
			break;
		}
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
static void FreeTokenAndStackMemory(JSON_TOKENS *tokens, JSON_DIVIDER_STACK *stack)
{
	for (u32 i = 0; i < tokens->tokenCount; ++i)
	{
		if (tokens->tokens[i].identifier != NULL)
			JSON_Deallocate(tokens->tokens[i].identifier);
	}
	JSON_Deallocate(tokens->tokens);
	JSON_Deallocate(tokens);

	JSON_Deallocate(stack->dividerStack);
}

// NOTE: @Jon
// Parses a JSON string
JSON *JSONLIB_ParseJSON(const char *jsonString, u32 stringLength)
{
	JSON_DIVIDER_STACK stack;
	stack.dividerStack = (char*)JSON_Allocate(sizeof(char) * JSON_DEFAULT_DIVIDER_STACK_SIZE);
	stack.dividerCount = 0;
	stack.dividerCapacity = JSON_DEFAULT_DIVIDER_STACK_SIZE;

	JSON_TOKENS *tokens = Tokenise(jsonString, stringLength, &stack);

	if (stack.dividerCount != 0)
	{
		FreeTokenAndStackMemory(tokens, &stack);
		return NULL;
	}

	if (!CorrectTokens(tokens, &stack))
	{
		FreeTokenAndStackMemory(tokens, &stack);
		return NULL;
	}

	JSON *json = ParseJSONInternal(tokens->tokens, tokens->tokenCount, &stack, JSON_OBJECT_TAG, NULL);

	if (json == NULL)
	{
		FreeTokenAndStackMemory(tokens, &stack);
		return NULL;
	}

	JSON_Deallocate(tokens->tokens);

	JSON_Deallocate(tokens);

	JSON_Deallocate(stack.dividerStack);

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

static const char *CopyStringValueToString(char *dest, const char *source, const u32 sourceLength)
{
	memcpy(dest, source, sizeof(char) * sourceLength);
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

static char *MakeValueString(const JSON *json, const u32 stringSize, JSON_DIVIDER_STACK *stack)
{
	char *valueString = NULL;

	if (json->tags & JSON_DECIMAL_TAG || json->tags & JSON_INTEGER_TAG || json->tags & JSON_BOOLEAN_TAG || json->tags & JSON_NULL_TAG)
		valueString = (char*)JSON_Allocate(sizeof(char) * stringSize);

	if (json->tags & JSON_DECIMAL_TAG)
		DecimalValueToString(valueString, json->decimal, stringSize);
	else if (json->tags & JSON_INTEGER_TAG)
		IntegerValueToString(valueString, json->integer, stringSize);
	else if (json->tags & JSON_BOOLEAN_TAG)
		BooleanValueToString(valueString, json->boolean);
	else if (json->tags & JSON_NULL_TAG)
		NullValueToString(valueString);
	else if (json->tags & JSON_STRING_TAG)
	{
		const u32 valueStrLen = strlen(json->string);
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

static JSON_STRING_STRUCT *MakeJSONPrettyNewline(JSON_STRING_STRUCT *str)
{
	StringCapacityHelper(str, 1);
	str->raw[str->length++] = '\n';
	return str;
}

static JSON_STRING_STRUCT *MakeJSONInternal(JSON_STRING_STRUCT *str, JSON_DIVIDER_STACK *stack, const JSON *const json, const bool humanReadable)
{
	if (json->name != NULL)
	{
		const u32 nameLen = strlen(json->name);
		if (nameLen > 0)
		{
			char* name = MakeStringValueString(json->name, nameLen);
			StringCapacityHelper(str, (u32)nameLen);
			memcpy(&str->raw[str->length], name, sizeof(char) + nameLen);
			str->length += nameLen;
			StringCapacityHelper(str, 1);
			str->raw[str->length++] = ':';
			StringCapacityHelper(str, 64);
			JSON_Deallocate(name);
		}
	}

	if (json->tags & JSON_OBJECT_TAG)
	{
		DividerStackPush(stack, '{');
		str->raw[str->length++] = '{';
		StringCapacityHelper(str, 0);
	}
	else if (json-> tags & JSON_ARRAY_TAG)
	{
		DividerStackPush(stack, '[');
		str->raw[str->length++] = '[';
		StringCapacityHelper(str, 0);
	}

	if (json->valueCount > 0)
	{
		for (u32 i = 0; i < json->valueCount; ++i)
		{
			MakeJSONInternal(str, stack, json->values[i], humanReadable);

			if (i != json->valueCount - 1)
			{
				StringCapacityHelper(str, 1);

				str->raw[str->length++] = ',';
			}

			if (humanReadable)
				MakeJSONPrettyNewline(str);
		}
	}
	else
	{
		// TODO: @Jon
		// Shouldn't hardcode the value string size!
		char* valString = MakeValueString(json, 64, stack);

		if (valString != NULL)
		{
			const u32 valStrLen = (u32)strlen(valString);
			memcpy(&str->raw[str->length], valString, sizeof(char) + valStrLen);

			str->length += valStrLen;

			JSON_Deallocate(valString);
		}
	}

	if (json->tags & JSON_OBJECT_TAG)
	{
		DividerStackPop(stack);
		str->raw[str->length++] = '}';
		StringCapacityHelper(str, 0);
	}
	else if (json->tags & JSON_ARRAY_TAG)
	{
		DividerStackPop(stack);
		str->raw[str->length++] = ']';
		StringCapacityHelper(str, 0);
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
	jsonString.raw = (char*)JSON_Allocate(sizeof(char) * 64);

	JSON_DIVIDER_STACK stack;
	stack.dividerStack = (char*)JSON_Allocate(sizeof(char) * JSON_DEFAULT_DIVIDER_STACK_SIZE);
	stack.dividerCount = 0;
	stack.dividerCapacity = JSON_DEFAULT_DIVIDER_STACK_SIZE;

	MakeJSONInternal(&jsonString, &stack, json, humanReadable);

	StringCapacityHelper(&jsonString, 1);

	jsonString.raw[jsonString.length++] = '\0';

	JSON_Deallocate(stack.dividerStack);

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

	if (json->tags & JSON_STRING_TAG)
		JSON_Deallocate((void*)json->string);

	if (json->valueCount > 0)
		JSON_Deallocate(json->values);

	JSON_Deallocate(json);
}