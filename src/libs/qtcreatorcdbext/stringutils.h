/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef SPLIT_H
#define SPLIT_H

#include <string>
#include <sstream>
#include <map>
#include <functional>

void trimFront(std::string &s); // Strip blanks off front
void trimBack(std::string &s);  // Strip blanks off back
// Simplify blanks, that is " A \tB " -> "A B".
void simplify(std::string &s);

// Split a token sequence in a string by character separator.
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

// A boolean predicate that can be used for grepping sequences
// of strings for a 'needle' substring.
class SubStringPredicate : public std::unary_function<const std::string &, bool>
{
public:
    explicit SubStringPredicate(const char *needle) : m_needle(needle) {}
    bool operator()(const std::string &s) { return s.find(m_needle) != std::string::npos; }

private:
    const char *m_needle;
};

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

// Helper for outputting a sequence/container to a ostream as a comma-separated, quoted list
// to be used as "os << DebugSequence<Iterator>(list.begin(), list.end()) << "..."
template <class Iterator>
struct DebugSequence : public std::pair<Iterator, Iterator>
{
    DebugSequence(const Iterator &i1, const Iterator &i2) : std::pair<Iterator, Iterator>(i1, i2) {}
};

template <class Iterator>
std::ostream &operator<<(std::ostream &os, const DebugSequence<Iterator> &s)
{
    for (Iterator it = s.first; it != s.second; ++it) {
        if (it != s.first)
            os << ',';
        os << '\'' << *it << '\'';
    }
    return os;
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

// Strings from raw data.
std::wstring quotedWStringFromCharData(const unsigned char *data, size_t size, bool truncated = false);
std::wstring quotedWStringFromWCharData(const unsigned char *data, size_t size, bool truncated = false);

// Helper for dumping memory
std::string dumpMemory(const unsigned char *data, size_t size, bool wantQuotes = true);

// String from hex "414A" -> "AJ".
std::string stringFromHex(const char *begin, const char *end);
// Decode hex to a memory area.
void decodeHex(const char *begin, const char *end, unsigned char *target);

std::wstring dataToHexW(const unsigned char *begin, const unsigned char *end);
std::string dataToHex(const unsigned char *begin, const unsigned char *end);
// Create readable hex: '0xAA 0xBB'..
std::wstring dataToReadableHexW(const unsigned char *begin, const unsigned char *end);

// Flat memory handle to pass allocated memory around.
class MemoryHandle {
    MemoryHandle(const MemoryHandle &);
    MemoryHandle& operator=(const MemoryHandle &);

public:
    explicit MemoryHandle(const unsigned char *memory, size_t size) : m_size(size), m_memory(memory) {}
    explicit MemoryHandle(const wchar_t *wcharData, size_t size) :
                 m_size(size * sizeof(wchar_t)),
                 m_memory(reinterpret_cast<const unsigned char *>(wcharData)) {}

    static MemoryHandle* fromStdString(const std::string &s);
    static MemoryHandle* fromStdWString(const std::wstring &s);

    ~MemoryHandle() { delete [] m_memory; }

    size_t size() const { return m_size; }
    const unsigned char *data() const { return m_memory; }
    const unsigned char *begin() const { return m_memory; }
    const unsigned char *end() const { return m_memory + m_size; }

    std::string dump(bool wantQuotes) const { return dumpMemory(begin(), m_size, wantQuotes); }
    std::wstring toHexW() const { return dataToHexW(begin(), end()); }
    std::string toHex() const { return dataToHex(begin(), end()); }
    std::wstring toReadableHexW() const { return dataToReadableHexW(begin(), end()); }

private:
    const size_t m_size;
    const unsigned char *m_memory;
};

// Format a map as a GDBMI hash {key="value",..}
void formatGdbmiHash(std::ostream &os, const std::map<std::string, std::string> &, bool closeHash = true);

#endif // SPLIT_H
