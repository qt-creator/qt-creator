/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include <clang-c/CXString.h>

#include <utf8string.h>

#include <cstring>
#include <ostream>

namespace ClangBackEnd {

class ClangString
{
public:
    ClangString(CXString cxString)
        : m_cxString(cxString)
    {
    }

    ~ClangString()
    {
        clang_disposeString(m_cxString);
    }

    ClangString(const ClangString &) = delete;
    const ClangString &operator=(const ClangString &) = delete;


    ClangString(ClangString &&other)
        : m_cxString(std::move(other.m_cxString))
    {
        other.m_cxString.data = nullptr;
        other.m_cxString.private_flags = 0;
    }


    ClangString &operator=(ClangString &&other)
    {
        if (this != &other) {
            clang_disposeString(m_cxString);
            m_cxString = std::move(other.m_cxString);
            other.m_cxString.data = nullptr;
            other.m_cxString.private_flags = 0;
        }

        return *this;
    }

    const char *cString() const
    {
        return clang_getCString(m_cxString);
    }

    operator Utf8String() const
    {
        return Utf8String(cString(), -1);
    }

    bool isNull() const
    {
        return m_cxString.data == nullptr;
    }

    bool hasContent() const
    {
        return !isNull() && std::strlen(cString()) > 0;
    }

    bool startsWith(const char* str) const
    {
        return std::strncmp(cString(), str, strlen(str)) == 0;
    }

    friend bool operator==(const ClangString &first, const ClangString &second)
    {
        return std::strcmp(first.cString(), second.cString()) == 0;
    }

    template<std::size_t Size>
    friend bool operator==(const ClangString &first, const char(&second)[Size])
    {
        return std::strncmp(first.cString(), second, Size) == 0; // Size includes \0
    }

    template<std::size_t Size>
    friend bool operator==(const char(&first)[Size], const ClangString &second)
    {
        return second == first;
    }

    template<typename Type,
             typename = typename std::enable_if<std::is_pointer<Type>::value>::type
             >
    friend bool operator==(const ClangString &first, Type second)
    {
        return std::strcmp(first.cString(), second) == 0;
    }

    template<typename Type,
             typename = typename std::enable_if<std::is_pointer<Type>::value>::type
             >
    friend bool operator==(Type first, const ClangString &second)
    {
        return second == first;
    }

    friend bool operator!=(const ClangString &first, const ClangString &second)
    {
        return !(first == second);
    }

    template<std::size_t Size>
    friend bool operator!=(const ClangString &first, const char(&second)[Size])
    {
        return !(first == second);
    }

    template<std::size_t Size>
    friend bool operator!=(const char(&first)[Size], const ClangString &second)
    {
        return second != first;
    }

    template<typename Type,
             typename = typename std::enable_if<std::is_pointer<Type>::value>::type
             >
    friend bool operator!=(const ClangString &first, Type second)
    {
        return !(first == second);
    }

    template<typename Type,
             typename = typename std::enable_if<std::is_pointer<Type>::value>::type
             >
    friend bool operator!=(Type first, const ClangString &second)
    {
        return !(first == second);
    }

    friend std::ostream &operator<<(std::ostream &os, const ClangString &string)
    {
        return os << string.cString();
    }

private:
    CXString m_cxString;
};

} // namespace ClangBackEnd
