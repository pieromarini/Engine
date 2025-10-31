#include "core_strings.h"

#include <cstdarg>

#define STB_SPRINTF_IMPLEMENTATION
#include "stb/stb_sprintf.h"

b32 charIsAlpha(u8 c) {
	return charIsAlphaUpper(c) || charIsAlphaLower(c);
}

b32 charIsAlphaUpper(u8 c) {
	return c >= 'A' && c <= 'Z';
}

b32 charIsAlphaLower(u8 c) {
	return c >= 'a' && c <= 'z';
}

b32 charIsDigit(u8 c) {
	return (c >= '0' && c <= '9');
}

b32 charIsSymbol(u8 c) {
	return (c == '~' || c == '!' || c == '$' || c == '%' || c == '^' || c == '&' || c == '*' || c == '-' || c == '=' || c == '+' || c == '<' || c == '.' || c == '>' || c == '/' || c == '?' || c == '|' || c == '\\' || c == '{' || c == '}' || c == '(' || c == ')' || c == '[' || c == ']' || c == '#' || c == ',' || c == ';' || c == ':' || c == '@');
}

b32 charIsSpace(u8 c) {
	return c == ' ' || c == '\r' || c == '\t' || c == '\f' || c == '\v' || c == '\n';
}

u8 charToUpper(u8 c) {
	return (c >= 'a' && c <= 'z') ? ('A' + (c - 'a')) : c;
}

u8 charToLower(u8 c) {
	return (c >= 'A' && c <= 'Z') ? ('a' + (c - 'A')) : c;
}

u8 charToForwardSlash(u8 c) {
	return (c == '\\' ? '/' : c);
}

u64 calculateCStringLength(const char* str) {
	u64 length = 0;
	for (; str[length]; length++);
	return length;
}

String8 Str8(u8* str, u64 size) {
	String8 string{};
	string.str = str;
	string.size = size;
	return string;
}

String8 Str8Range(u8* first, u8* one_past_last) {
	String8 string{};
	string.str = first;
	string.size = (u64)(one_past_last - first);
	return string;
}

String16 Str16(u16* str, u64 size) {
	String16 result{};
	result.str = str;
	result.size = size;
	return result;
}

String32 Str32(u32* str, u64 size) {
	String32 string = { .str = nullptr };
	string.str = str;
	string.size = size;
	return string;
}

String8 PushStr8Copy(Arena* arena, String8 string) {
	String8 res{};
	res.size = string.size;
	res.str = PushArrayNoZero(arena, u8, string.size + 1);
	MemoryCopy(res.str, string.str, string.size);
	res.str[string.size] = 0;
	return res;
}

String8 PushStr8FV(Arena* arena, const char* fmt, va_list args) {
	String8 result = { .str = nullptr };
	va_list args2 = nullptr;
	va_copy(args2, args);
	u64 neededBytes = stbsp_vsnprintf(nullptr, 0, fmt, args) + 1;
	result.str = PushArrayNoZero(arena, u8, neededBytes);
	result.size = neededBytes - 1;
	stbsp_vsnprintf((char*)result.str, neededBytes, fmt, args2);
	return result;
}

String8 PushStr8F(Arena* arena, const char* fmt, ...) {
	String8 result = { .str = nullptr };
	va_list args = nullptr;
	va_start(args, fmt);
	result = PushStr8FV(arena, fmt, args);
	va_end(args);
	return result;
}

String8 PushStr8FillByte(Arena* arena, u64 size, u8 byte) {
	String8 result = { .str = nullptr };
	result.str = PushArrayNoZero(arena, u8, size);
	MemorySet(result.str, byte, size);
	result.size = size;
	return result;
}

String8 Str8Skip(String8 str, u64 min) {
	return Substr8(str, Rect1D(min, str.size));
}

b32 Str8Match(String8 a, String8 b, MatchFlags flags) {
	b32 result = 0;
	if (a.size == b.size || flags & MatchFlag_RightSideSloppy) {
		result = 1;
		for (u64 i = 0; i < a.size; i += 1) {
			b32 match = (a.str[i] == b.str[i]);
			if (flags & MatchFlag_CaseInsensitive) {
				match |= (charToLower(a.str[i]) == charToLower(b.str[i]));
			}
			if (flags & MatchFlag_SlashInsensitive) {
				match |= (charToForwardSlash(a.str[i]) == charToForwardSlash(b.str[i]));
			}
			if (match == 0) {
				result = 0;
				break;
			}
		}
	}
	return result;
}

String8 Substr8(String8 str, Rect1D rect) {
	auto min = rect.min;
	auto max = rect.max;
	if (max > str.size) {
		max = str.size;
	}
	if (min > str.size) {
		min = str.size;
	}
	if (min > max) {
		u64 swap = min;
		min = max;
		max = swap;
	}
	str.size = max - min;
	str.str += min;
	return str;
}

u64 FindSubstr8(String8 haystack, String8 needle, u64 startPos, MatchFlags flags) {
	b32 found = 0;
	u64 found_idx = haystack.size;
	for (u64 i = startPos; i < haystack.size; i += 1) {
		if (i + needle.size <= haystack.size) {
			String8 substr = Substr8(haystack, Rect1D(i, i + needle.size));
			if (Str8Match(substr, needle, flags)) {
				found_idx = i;
				found = 1;
				if (!(flags & MatchFlag_FindLast)) {
					break;
				}
			}
		}
	}
	return found_idx;
}

static const u8 utf8_class[32] = {
 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,2,2,2,2,3,3,4,5,
};

#define bitmask1 0x01
#define bitmask2 0x03
#define bitmask3 0x07
#define bitmask4 0x0F
#define bitmask5 0x1F
#define bitmask6 0x3F
#define bitmask7 0x7F
#define bitmask8 0xFF
#define bitmask9 0x01FF
#define bitmask10 0x03FF

DecodedCodepoint DecodeCodepointFromUtf8(u8* str, u64 max) {
	DecodedCodepoint result = { ~((u32)0), 1 };
	u8 byte = str[0];
	u8 byte_class = utf8_class[byte >> 3];
	switch (byte_class) {
	case 1: {
		result.codepoint = byte;
	} break;

	case 2: {
		if (2 <= max) {
			u8 cont_byte = str[1];
			if (utf8_class[cont_byte >> 3] == 0) {
				result.codepoint = (byte & bitmask5) << 6;
				result.codepoint |= (cont_byte & bitmask6);
				result.advance = 2;
			}
		}
	} break;

	case 3: {
		if (3 <= max) {
			u8 cont_byte[2] = { str[1], str[2] };
			if (utf8_class[cont_byte[0] >> 3] == 0 && utf8_class[cont_byte[1] >> 3] == 0) {
				result.codepoint = (byte & bitmask4) << 12;
				result.codepoint |= ((cont_byte[0] & bitmask6) << 6);
				result.codepoint |= (cont_byte[1] & bitmask6);
				result.advance = 3;
			}
		}
	} break;

	case 4: {
		if (4 <= max) {
			u8 cont_byte[3] = { str[1], str[2], str[3] };
			if (utf8_class[cont_byte[0] >> 3] == 0 && utf8_class[cont_byte[1] >> 3] == 0 && utf8_class[cont_byte[2] >> 3] == 0) {
				result.codepoint = (byte & bitmask3) << 18;
				result.codepoint |= ((cont_byte[0] & bitmask6) << 12);
				result.codepoint |= ((cont_byte[1] & bitmask6) << 6);
				result.codepoint |= (cont_byte[2] & bitmask6);
				result.advance = 4;
			}
		}
	} break;
	}

	return result;
}
u32 Utf16FromCodepoint(u16* out, u32 codepoint) {
	u32 advance = 1;
	if (codepoint == ~((u32)0)) {
		out[0] = (u16)'?';
	} else if (codepoint < 0x10000) {
		out[0] = (u16)codepoint;
	} else {
		u64 v = codepoint - 0x10000;
		out[0] = 0xD800 + (v >> 10);
		out[1] = 0xDC00 + (v & bitmask10);
		advance = 2;
	}
	return advance;
}

String16 Str16From8(Arena* arena, String8 in) {
	u64 cap = in.size * 2;
	u16* str = PushArrayNoZero(arena, u16, cap + 1);
	u8* ptr = in.str;
	u8* opl = ptr + in.size;
	u64 size = 0;
	DecodedCodepoint consume{};
	for (; ptr < opl;) {
		consume = DecodeCodepointFromUtf8(ptr, opl - ptr);
		ptr += consume.advance;
		size += Utf16FromCodepoint(str + size, consume.codepoint);
	}
	str[size] = 0;
	arenaPop(arena, 2 * (cap - size));
	String16 result = { .str = str, .size = size };
	return result;
}


void Str8ListPushNode(String8List* list, String8Node* node) {
	QueuePush(list->first, list->last, node);
	list->count++;
	list->totalSize += node->string.size;
}

void Str8ListPush(Arena* arena, String8List* list, String8 str) {
	String8Node* node = PushStruct(arena, String8Node);
	node->string = str;
	Str8ListPushNode(list, node);
}

void Str8ListPushF(Arena* arena, String8List* list, char* fmt, ...) {
	va_list args{};
	va_start(args, fmt);
	String8 str = PushStr8FV(arena, fmt, args);
	va_end(args);
	Str8ListPush(arena, list, str);
}


String8Array Str8ArrayFromList(Arena* arena, String8List list) {
	String8Array arr{};
	arr.count = list.count;
	arr.strings = PushArrayNoZero(arena, String8, list.count);

	u64 idx = 0;
	for (String8Node* n = list.first; n != nullptr; n = n->next, idx++) {
		arr.strings[idx] = n->string;
	}

	return arr;
}

CStringArray CStringArrayFromList(Arena* arena, String8List list) {
	CStringArray arr{};
	arr.count = list.count;
	arr.strings = PushArray(arena, char*, list.count);

	u64 idx = 0;
	for (String8Node* n = list.first; n != nullptr; n = n->next, idx++) {
		arr.strings[idx] = (char*)PushStr8Copy(arena, n->string).str;
	}

	return arr;
}
