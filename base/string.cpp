// LAF Base Library
// Copyright (c) 2020-2024 Igara Studio S.A.
// Copyright (c) 2001-2016 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "base/debug.h"
#include "base/string.h"
#include "base/utf8_decode.h"

#include <cctype>
#include <vector>

#ifdef LAF_WINDOWS
  #include <windows.h>
#endif

namespace base {

// Based on Allegro Unicode code (allegro/src/unicode.c)
static std::size_t insert_utf8_char(const codepoint_t chr, std::string* result = nullptr)
{
  int size, bits, b, i;

  if (chr < 128) {
    if (result)
      result->push_back(chr);
    return 1;
  }

  bits = 7;
  while (chr >= (1 << bits))
    bits++;

  size = 2;
  b = 11;

  while (b < bits) {
    size++;
    b += 5;
  }

  if (result) {
    b -= (7 - size);
    int firstbyte = chr >> b;
    for (i = 0; i < size; i++)
      firstbyte |= (0x80 >> i);

    result->push_back(firstbyte);

    for (i = 1; i < size; i++) {
      b -= 6;
      result->push_back(0x80 | ((chr >> b) & 0x3F));
    }
  }

  return size;
}

std::string string_printf(const char* format, ...)
{
  std::va_list ap;
  va_start(ap, format);
  std::string result = string_vprintf(format, ap);
  va_end(ap);
  return result;
}

std::string string_vprintf(const char* format, va_list ap)
{
  std::vector<char> buf(1, 0);
  std::va_list ap2;
  va_copy(ap2, ap);
  const size_t required_size = std::vsnprintf(nullptr, 0, format, ap);
  if (required_size > 0) {
    buf.resize(required_size + 1);
    std::vsnprintf(buf.data(), buf.size(), format, ap2);
  }
  va_end(ap2);
  return std::string(buf.data());
}

std::string string_to_lower(const std::string& original)
{
  std::wstring result(from_utf8(original));
  auto it(result.begin());
  auto end(result.end());
  while (it != end) {
    *it = std::tolower(*it);
    ++it;
  }
  return to_utf8(result);
}

std::string string_to_upper(const std::string& original)
{
  std::wstring result(from_utf8(original));
  auto it(result.begin());
  auto end(result.end());
  while (it != end) {
    *it = std::toupper(*it);
    ++it;
  }
  return to_utf8(result);
}

std::string codepoint_to_utf8(const codepoint_t codepoint)
{
  // TODO We could use std::c32rtomb() but macOS (10.9) doesn't have it.

  // Get required size to reserve a string so string::push_back()
  // doesn't need to reallocate its data.
  const std::size_t required_size = insert_utf8_char(codepoint);
  if (required_size == 0)
    return std::string();

  std::string result;
  result.reserve(required_size + 1);
  insert_utf8_char(codepoint, &result);
  return result;
}

codepoint_t utf16_to_codepoint(const uint16_t low, const uint16_t hi)
{
  // Valid code point
  if ((low >= 0x0000 && low <= 0xD7FF) || (low >= 0xE000 && low <= 0xFFFF)) {
    return codepoint_t(low);
  }

  // Surrogate pair
  if (low >= 0xDC00 && low <= 0xDFFF) {
    ASSERT(hi >= 0xD800 && hi <= 0xDBFF);
    return (0x10000 | (((low - 0xDC00)) | ((hi - 0xD800) << 10)));
  }

  return 0;
}

#ifdef LAF_WINDOWS

std::string to_utf8(const wchar_t* src, const size_t n)
{
  int required_size = ::WideCharToMultiByte(CP_UTF8, 0, src, (int)n, NULL, 0, NULL, NULL);

  if (required_size == 0)
    return std::string();

  std::vector<char> buf(++required_size);

  ::WideCharToMultiByte(CP_UTF8, 0, src, (int)n, &buf[0], required_size, NULL, NULL);

  return std::string(&buf[0]);
}

std::wstring from_utf8(const std::string& src)
{
  int required_size = MultiByteToWideChar(CP_UTF8, 0, src.c_str(), (int)src.size(), NULL, 0);

  if (required_size == 0)
    return std::wstring();

  std::vector<wchar_t> buf(++required_size);

  ::MultiByteToWideChar(CP_UTF8, 0, src.c_str(), (int)src.size(), &buf[0], required_size);

  return std::wstring(&buf[0]);
}

#else

std::string to_utf8(const wchar_t* src, const size_t n)
{
  // Get required size to reserve a string so string::push_back()
  // doesn't need to reallocate its data.
  std::size_t required_size = 0;
  const auto* p = src;
  for (size_t i = 0; i < n; ++i, ++p)
    required_size += insert_utf8_char(*p);
  if (!required_size)
    return "";

  std::string result;
  result.reserve(++required_size);
  p = src;
  for (int i = 0; i < n; ++i, ++p)
    insert_utf8_char(*p, &result);
  return result;
}

std::wstring from_utf8(const std::string& src)
{
  int required_size = utf8_length(src);
  std::vector<wchar_t> buf(++required_size);
  std::vector<wchar_t>::iterator buf_it = buf.begin();
  #ifdef _DEBUG
  std::vector<wchar_t>::iterator buf_end = buf.end();
  #endif
  utf8_decode decode(src);

  while (const int chr = decode.next()) {
    ASSERT(buf_it != buf_end);
    *buf_it = chr;
    ++buf_it;
  }

  return std::wstring(buf.data());
}

#endif

int utf8_length(const std::string& utf8string)
{
  utf8_decode decode(utf8string);
  int c = 0;

  while (decode.next())
    ++c;

  return c;
}

int utf8_icmp(const std::string& a, const std::string& b, int n)
{
  utf8_decode a_decode(a);
  utf8_decode b_decode(b);
  int i = 0;

  for (; (n == 0 || i < n) && !a_decode.is_end() && !b_decode.is_end(); ++i) {
    int a_chr = a_decode.next();
    if (!a_chr)
      break;

    int b_chr = b_decode.next();
    if (!b_chr)
      break;

    a_chr = std::tolower(a_chr);
    b_chr = std::tolower(b_chr);

    if (a_chr < b_chr)
      return -1;
    if (a_chr > b_chr)
      return 1;
  }

  if (n > 0 && i == n)
    return 0;
  if (a_decode.is_end() && b_decode.is_end())
    return 0;
  if (a_decode.is_end())
    return -1;
  return 1;
}

bool is_dead_key(codepoint_t ch)
{
  return (ch == 0x00B4 || // ´
          ch == 0x0027 || // '
          ch == 0x0060 || // `
          ch == 0x00A8 || // ¨
          ch == 0x0022 || // "
          ch == 0x02C6 || // ˆ
          ch == 0x02DC || // ˜
          ch == 0x00AF || // ¯
          ch == 0x02C9 || // ˉ
          ch == 0x02D8 || // ˘
          ch == 0x02C7 || // ˇ
          ch == 0x02DA || // ˚
          ch == 0x00B0 || // °
          ch == 0x00B8 || // ¸
          ch == 0x02DB || // ˛
          ch == 0x02DD);  // ˝
}

codepoint_t compose_dead_key(codepoint_t deadKey, codepoint_t baseChar)
{
  if (baseChar == deadKey)
    return deadKey;
  if (deadKey == 0x00B4 || deadKey == 0x0027) { // ´ or '
    switch (baseChar) {
      case 'a':
      case 'A': return baseChar == 'a' ? 0x00E1 : 0x00C1; // á Á
      case 'e':
      case 'E': return baseChar == 'e' ? 0x00E9 : 0x00C9; // é É
      case 'i':
      case 'I': return baseChar == 'i' ? 0x00ED : 0x00CD; // í Í
      case 'o':
      case 'O': return baseChar == 'o' ? 0x00F3 : 0x00D3; // ó Ó
      case 'u':
      case 'U': return baseChar == 'u' ? 0x00FA : 0x00DA; // ú Ú
      case 'y':
      case 'Y': return baseChar == 'y' ? 0x00FD : 0x00DD; // ý Ý
      case ' ': return deadKey;
    }
  }
  else if (deadKey == 0x0060) { // `
    switch (baseChar) {
      case 'a':
      case 'A': return baseChar == 'a' ? 0x00E0 : 0x00C0; // à À
      case 'e':
      case 'E': return baseChar == 'e' ? 0x00E8 : 0x00C8; // è È
      case 'i':
      case 'I': return baseChar == 'i' ? 0x00EC : 0x00CC; // ì Ì
      case 'o':
      case 'O': return baseChar == 'o' ? 0x00F2 : 0x00D2; // ò Ò
      case 'u':
      case 'U': return baseChar == 'u' ? 0x00F9 : 0x00D9; // ù Ù
      case ' ': return deadKey;
    }
  }
  else if (deadKey == 0x00A8 || deadKey == 0x0022) { // ¨ or "
    switch (baseChar) {
      case 'a':
      case 'A': return baseChar == 'a' ? 0x00E4 : 0x00C4; // ä Ä
      case 'e':
      case 'E': return baseChar == 'e' ? 0x00EB : 0x00CB; // ë Ë
      case 'i':
      case 'I': return baseChar == 'i' ? 0x00EF : 0x00CF; // ï Ï
      case 'o':
      case 'O': return baseChar == 'o' ? 0x00F6 : 0x00D6; // ö Ö
      case 'u':
      case 'U': return baseChar == 'u' ? 0x00FC : 0x00DC; // ü Ü
      case 'y': return 0x00FF;                            // ÿ
      case ' ': return deadKey;
    }
  }
  else if (deadKey == 0x02C6) { // ^
    switch (baseChar) {
      case 'a':
      case 'A': return baseChar == 'a' ? 0x00E2 : 0x00C2; // â Â
      case 'e':
      case 'E': return baseChar == 'e' ? 0x00EA : 0x00CA; // ê Ê
      case 'i':
      case 'I': return baseChar == 'i' ? 0x00EE : 0x00CE; // î Î
      case 'o':
      case 'O': return baseChar == 'o' ? 0x00F4 : 0x00D4; // ô Ô
      case 'u':
      case 'U': return baseChar == 'u' ? 0x00FB : 0x00DB; // û Û
      case ' ': return 0x005E;
    }
  }
  else if (deadKey == 0x02DC) { // ~
    switch (baseChar) {
      case 'a':
      case 'A': return baseChar == 'a' ? 0x00E3 : 0x00C3; // ã Ã
      case 'n':
      case 'N': return baseChar == 'n' ? 0x00F1 : 0x00D1; // ñ Ñ
      case 'o':
      case 'O': return baseChar == 'o' ? 0x00F5 : 0x00D5; // õ Õ
      case ' ': return 0x007E;
    }
  }
  else if (deadKey == 0x00AF || deadKey == 0x02C9) { // ¯ or ˉ
    switch (baseChar) {
      case 'a':
      case 'A': return baseChar == 'a' ? 0x0101 : 0x0100; // ā Ā
      case 'e':
      case 'E': return baseChar == 'e' ? 0x0113 : 0x0112; // ē Ē
      case 'i':
      case 'I': return baseChar == 'i' ? 0x012B : 0x012A; // ī Ī
      case 'o':
      case 'O': return baseChar == 'o' ? 0x014D : 0x014C; // ō Ō
      case 'u':
      case 'U': return baseChar == 'u' ? 0x016B : 0x016A; // ū Ū
      case 'y':
      case 'Y': return baseChar == 'y' ? 0x0233 : 0x0232; // ȳ Ȳ
      case ' ': return 0x00AF;
    }
  }
  else if (deadKey == 0x02D8) { // ˘
    switch (baseChar) {
      case 'a':
      case 'A': return baseChar == 'a' ? 0x0103 : 0x0102; // ă Ă
      case 'e':
      case 'E': return baseChar == 'e' ? 0x0115 : 0x0114; // ĕ Ĕ
      case 'g':
      case 'G': return baseChar == 'g' ? 0x011F : 0x011E; // ğ Ğ
      case 'i':
      case 'I': return baseChar == 'i' ? 0x012D : 0x012C; // ĭ Ĭ
      case 'o':
      case 'O': return baseChar == 'o' ? 0x014F : 0x014E; // ŏ Ŏ
      case 'u':
      case 'U': return baseChar == 'u' ? 0x016D : 0x016C; // ŭ Ŭ
      case ' ': return deadKey;
    }
  }
  else if (deadKey == 0x02C7) { // ˇ
    switch (baseChar) {
      case 'a':
      case 'A': return baseChar == 'a' ? 0x01CE : 0x01CD; // ǎ Ǎ
      case 'c':
      case 'C': return baseChar == 'c' ? 0x010D : 0x010C; // č Č
      case 'd':
      case 'D': return baseChar == 'd' ? 0x010F : 0x010E; // ď Ď
      case 'e':
      case 'E': return baseChar == 'e' ? 0x011B : 0x011A; // ě Ě
      case 'g':
      case 'G': return baseChar == 'g' ? 0x01E7 : 0x01E6; // ǧ Ǧ
      case 'i':
      case 'I': return baseChar == 'i' ? 0x01D0 : 0x01CF; // ǐ Ǐ
      case 'j':
      case 'J': return baseChar == 'j' ? 0x01F0 : 0x01F0; // ǰ (no uppercase)
      case 'k':
      case 'K': return baseChar == 'k' ? 0x01E9 : 0x01E8; // ǩ Ǩ
      case 'l':
      case 'L': return baseChar == 'l' ? 0x013E : 0x013D; // ľ Ľ
      case 'n':
      case 'N': return baseChar == 'n' ? 0x0148 : 0x0147; // ň Ň
      case 'o':
      case 'O': return baseChar == 'o' ? 0x01D2 : 0x01D1; // ǒ Ǒ
      case 'r':
      case 'R': return baseChar == 'r' ? 0x0159 : 0x0158; // ř Ř
      case 's':
      case 'S': return baseChar == 's' ? 0x0161 : 0x0160; // š Š
      case 't':
      case 'T': return baseChar == 't' ? 0x0165 : 0x0164; // ť Ť
      case 'u':
      case 'U': return baseChar == 'u' ? 0x01D4 : 0x01D3; // ǔ Ǔ
      case 'z':
      case 'Z': return baseChar == 'z' ? 0x017E : 0x017D; // ž Ž
      case ' ': return deadKey;
    }
  }
  else if (deadKey == 0x02DA || deadKey == 0x00B0) { // ˚ or °
    switch (baseChar) {
      case 'a':
      case 'A': return baseChar == 'a' ? 0x00E5 : 0x00C5; // å Å
      case 'u':
      case 'U': return baseChar == 'u' ? 0x016F : 0x016E; // ů Ů
      case 'w':
      case 'W': return baseChar == 'w' ? 0x1E98 : 0x1E98; // ẘ (no uppercase)
      case 'y':
      case 'Y': return baseChar == 'y' ? 0x1E99 : 0x1E99; // ẙ (no uppercase)
      case ' ': return 0x00B0;
    }
  }
  else if (deadKey == 0x00B8) { // ¸
    switch (baseChar) {
      case 'c':
      case 'C': return baseChar == 'c' ? 0x00E7 : 0x00C7; // ç Ç
      case 'd':
      case 'D': return baseChar == 'd' ? 0x1E11 : 0x1E10; // ḑ Ḑ
      case 'e':
      case 'E': return baseChar == 'e' ? 0x0229 : 0x0228; // ȩ Ȩ
      case 'g':
      case 'G': return baseChar == 'g' ? 0x0123 : 0x0122; // ģ Ģ
      case 'h':
      case 'H': return baseChar == 'h' ? 0x1E29 : 0x1E28; // ḩ Ḩ
      case 'k':
      case 'K': return baseChar == 'k' ? 0x0137 : 0x0136; // ķ Ķ
      case 'l':
      case 'L': return baseChar == 'l' ? 0x013C : 0x013B; // ļ Ļ
      case 'n':
      case 'N': return baseChar == 'n' ? 0x0146 : 0x0145; // ņ Ņ
      case 'r':
      case 'R': return baseChar == 'r' ? 0x0157 : 0x0156; // ŗ Ŗ
      case 's':
      case 'S': return baseChar == 's' ? 0x015F : 0x015E; // ş Ş
      case 't':
      case 'T': return baseChar == 't' ? 0x0163 : 0x0162; // ţ Ţ
      case ' ': return deadKey;
    }
  }
  else if (deadKey == 0x02DB) { // ˛
    switch (baseChar) {
      case 'a':
      case 'A': return baseChar == 'a' ? 0x0105 : 0x0104; // ą Ą
      case 'e':
      case 'E': return baseChar == 'e' ? 0x0119 : 0x0118; // ę Ę
      case 'i':
      case 'I': return baseChar == 'i' ? 0x012F : 0x012E; // į Į
      case 'o':
      case 'O': return baseChar == 'o' ? 0x01EB : 0x01EA; // ǫ Ǫ
      case 'u':
      case 'U': return baseChar == 'u' ? 0x0173 : 0x0172; // ų Ų
      case ' ': return deadKey;
    }
  }
  else if (deadKey == 0x02DD) { // ˝
    switch (baseChar) {
      case 'o':
      case 'O': return baseChar == 'o' ? 0x0151 : 0x0150; // ő Ő
      case 'u':
      case 'U': return baseChar == 'u' ? 0x0171 : 0x0170; // ű Ű
      case ' ': return deadKey;
    }
  }

  return 0;
}

} // namespace base
