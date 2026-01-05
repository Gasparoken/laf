// LAF Base Library
// Copyright (c) 2020-2025 Igara Studio S.A.
// Copyright (c) 2001-2017 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef BASE_STRING_H_INCLUDED
#define BASE_STRING_H_INCLUDED
#pragma once

#include <cstdarg>
#include <iterator>
#include <string>

#include "base/codepoint.h"
#include "base/ints.h"

namespace base {

std::string string_printf(const char* format, ...);
std::string string_vprintf(const char* format, std::va_list ap);

std::string string_to_lower(const std::string& original);
std::string string_to_upper(const std::string& original);

std::string codepoint_to_utf8(codepoint_t codepoint);
codepoint_t utf16_to_codepoint(uint16_t low, uint16_t hi);

std::string to_utf8(const wchar_t* src, size_t n);

inline std::string to_utf8(const std::wstring& widestring)
{
  return to_utf8(widestring.c_str(), widestring.size());
}

std::wstring from_utf8(const std::string& utf8string);

int utf8_length(const std::string& utf8string);
int utf8_icmp(const std::string& a, const std::string& b, int n = 0);

bool is_dead_key(codepoint_t ch);
codepoint_t compose_dead_key(codepoint_t deadKey, codepoint_t baseChar);

} // namespace base

#endif
