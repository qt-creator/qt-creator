/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef SPLIT_H
#define SPLIT_H

#include <string>
#include <sstream>
#include <map>

void trimFront(std::string &s);
void trimBack(std::string &s);
void simplify(std::string &s);

// Split by character separator.
template <class Iterator>
void split(const std::string &s, char sep, Iterator it)
{
    const std::string::size_type size = s.size();
    for (std::string::size_type pos = 0; pos < size; ) {
        std::string::size_type nextpos = s.find(sep, pos);
        if (nextpos == std::string::npos)
            nextpos = size;
        const std::string token = s.substr(pos, nextpos - pos);
        *it = token;
        ++it;
        pos = nextpos + 1;
    }
}

// Format numbers, etc, as a string.
template <class Streamable>
std::string toString(const Streamable s)
{
    std::ostringstream str;
    str << s;
    return str.str();
}

// Format numbers, etc, as a wstring.
template <class Streamable>
std::wstring toWString(const Streamable s)
{
    std::wostringstream str;
    str << s;
    return str.str();
}

bool endsWith(const std::string &haystack, const char *needle);
inline bool endsWith(const std::string &haystack, char needle)
    { return !haystack.empty() && haystack.at(haystack.size() - 1) == needle; }

// Read an integer from a string as '10' or '0xA'
template <class Integer>
bool integerFromString(const std::string &s, Integer *v)
{
    const bool isHex = s.compare(0, 2, "0x") == 0;
    std::istringstream str(isHex ? s.substr(2, s.size() - 2) : s);
    if (isHex)
        str >> std::hex;
    str >> *v;
    return !str.fail();
}

// Read an integer from a wstring as '10' or '0xA'
template <class Integer>
bool integerFromWString(const std::wstring &s, Integer *v)
{
    const bool isHex = s.compare(0, 2, L"0x") == 0;
    std::wistringstream str(isHex ? s.substr(2, s.size() - 2) : s);
    if (isHex)
        str >> std::hex;
    str >> *v;
    return !str.fail();
}

void replace(std::wstring &s, wchar_t before, wchar_t after);

// Stream  a string onto a char stream doing backslash & octal escaping
// suitable for GDBMI usable as 'str << gdbmiStringFormat(wstring)'
class gdbmiStringFormat {
public:
    explicit gdbmiStringFormat(const std::string &s) : m_s(s) {}

    void format(std::ostream &) const;

private:
    const std::string &m_s;
};

// Stream  a wstring onto a char stream doing backslash & octal escaping
// suitable for GDBMI usable as 'str << gdbmiWStringFormat(wstring)'
class gdbmiWStringFormat {
public:
    explicit gdbmiWStringFormat(const std::wstring &w) : m_w(w) {}

    void format(std::ostream &) const;

private:
    const std::wstring &m_w;
};

inline std::ostream &operator<<(std::ostream &str, const gdbmiStringFormat &sf)
{
    sf.format(str);
    return str;
}

inline std::ostream &operator<<(std::ostream &str, const gdbmiWStringFormat &wsf)
{
    wsf.format(str);
    return str;
}

std::string wStringToGdbmiString(const std::wstring &w);
std::string wStringToString(const std::wstring &w);
std::wstring stringToWString(const std::string &w);

// String from hex "414A" -> "AJ".
std::string stringFromHex(const char *begin, const char *end);
std::wstring dataToHexW(const unsigned char *begin, const unsigned char *end);
// Create readable hex: '0xAA 0xBB'..
std::wstring dataToReadableHexW(const unsigned char *begin, const unsigned char *end);

// Format a map as a GDBMI hash {key="value",..}
void formatGdbmiHash(std::ostream &os, const std::map<std::string, std::string> &);

#endif // SPLIT_H
