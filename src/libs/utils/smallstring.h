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

#include "utils_global.h"

#include "smallstringliteral.h"
#include "smallstringiterator.h"
#include "smallstringview.h"
#include "smallstringmemory.h"

#include <QByteArray>
#include <QDataStream>
#include <QDebug>
#include <QString>

#include <algorithm>
#include <cstdlib>
#include <climits>
#include <cstring>
#include <iosfwd>
#include <utility>

#pragma push_macro("constexpr")
#ifndef __cpp_constexpr
#define constexpr
#endif

#pragma push_macro("noexcept")
#ifndef __cpp_noexcept
#define noexcept
#endif

#ifdef UNIT_TESTS
#define UNIT_TEST_PUBLIC public
#else
#define UNIT_TEST_PUBLIC private
#endif

namespace Utils {

class SmallString
{
public:
    using iterator = Internal::SmallStringIterator<std::random_access_iterator_tag, char>;
    using const_iterator = Internal::SmallStringIterator<std::random_access_iterator_tag, const char>;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;
    using size_type = std::size_t;

    SmallString() noexcept
        : m_data(Internal::StringDataLayout())
    {
    }

    constexpr
    SmallString(const SmallStringLiteral &stringReference)
        : m_data(stringReference.m_data)
    {
    }

    template<size_type Size>
    constexpr
    SmallString(const char(&string)[Size])
        : m_data(string)
    {
    }

    SmallString(const char *string, size_type size, size_type capacity)
    {
        if (Q_LIKELY(capacity <= shortStringCapacity())) {
            std::memcpy(m_data.shortString.string, string, size);
            m_data.shortString.string[size] = 0;
            m_data.shortString.shortStringSize = uchar(size);
            m_data.shortString.isReference = false;
            m_data.shortString.isReadOnlyReference = false;
        } else {
            m_data.allocated.data.pointer = Memory::allocate(capacity + 1);
            std::memcpy(m_data.allocated.data.pointer, string, size);
            m_data.allocated.data.pointer[size] = 0;
            m_data.allocated.data.size = size;
            m_data.allocated.data.capacity = capacity;
            m_data.allocated.shortStringSize = 0;
            m_data.allocated.isReference = true;
            m_data.allocated.isReadOnlyReference = false;
        }
    }

    SmallString(const char *string, size_type size)
        : SmallString(string, size, size)
    {
    }

    SmallString(const QString &qString)
        : SmallString(SmallString::fromQString(qString))
    {}

    ~SmallString() noexcept
    {
        if (Q_UNLIKELY(hasAllocatedMemory()))
            Memory::deallocate(m_data.allocated.data.pointer);
    }

#if !defined(UNIT_TESTS) && !(defined(_MSC_VER) && _MSC_VER < 1900)
    SmallString(const SmallString &other) = delete;
    SmallString &operator=(const SmallString &other) = delete;
#else
    SmallString(const SmallString &string)
    {
        if (string.isShortString() || string.isReadOnlyReference())
            m_data = string.m_data;
        else
            new (this) SmallString{string.data(), string.size()};
    }

    SmallString &operator=(const SmallString &other)
    {
        SmallString copy = other;

        swap(*this, copy);

        return *this;
    }
#endif

    SmallString(SmallString &&other) noexcept
    {
        m_data = other.m_data;
        other.m_data = Internal::StringDataLayout();
    }

    SmallString &operator=(SmallString &&other) noexcept
    {
        swap(*this, other);

        return *this;
    }

    SmallString clone() const
    {
        SmallString clonedString(m_data);

        if (Q_UNLIKELY(hasAllocatedMemory()))
            new (&clonedString) SmallString{m_data.allocated.data.pointer, m_data.allocated.data.size};

        return clonedString;
    }

    friend
    void swap(SmallString &first, SmallString &second)
    {
        using std::swap;

        swap(first.m_data, second.m_data);
    }

    QByteArray toQByteArray() const noexcept
    {
        return QByteArray(data(), int(size()));
    }

    QString toQString() const
    {
        return QString::fromUtf8(data(), int(size()));
    }

    operator SmallStringView() const
    {
        return SmallStringView(data(), size());
    }

    operator QString() const
    {
        return toQString();
    }

    static
    SmallString fromUtf8(const char *characterPointer)
    {
        return SmallString(characterPointer, std::strlen(characterPointer));
    }

    void reserve(size_type newCapacity)
    {
        if (fitsNotInCapacity(newCapacity)) {
            if (Q_UNLIKELY(hasAllocatedMemory())) {
                m_data.allocated.data.pointer = Memory::reallocate(m_data.allocated.data.pointer,
                                                               newCapacity + 1);
                m_data.allocated.data.capacity = newCapacity;
            } else {
                const size_type oldSize = size();

                new (this) SmallString(data(), oldSize, newCapacity);
            }
        }
    }

    void resize(size_type newSize)
    {
        reserve(newSize);
        setSize(newSize);
        at(newSize) = 0;
    }

    void clear() noexcept
    {
        this->~SmallString();
        m_data = Internal::StringDataLayout();
    }

    char *data() noexcept
    {
        return Q_LIKELY(isShortString()) ? m_data.shortString.string : m_data.allocated.data.pointer;
    }

    const char *data() const noexcept
    {
        return Q_LIKELY(isShortString()) ? m_data.shortString.string : m_data.allocated.data.pointer;
    }

    const char *constData() const noexcept
    {
        return data();
    }

    iterator begin() noexcept
    {
        return data();
    }

    iterator end() noexcept
    {
        return data() + size();
    }

    reverse_iterator rbegin() noexcept
    {
        return reverse_iterator(end() - 1l);
    }

    reverse_iterator rend() noexcept
    {
        return reverse_iterator(begin() - 1l);
    }

    const_iterator begin() const noexcept
    {
        return constData();
    }

    const_iterator end() const noexcept
    {
        return data() + size();
    }

    const_iterator cbegin() const noexcept
    {
        return begin();
    }

    const_iterator cend() const noexcept
    {
        return end();
    }

    static
    SmallString fromQString(const QString &qString)
    {
        const QByteArray &utf8ByteArray = qString.toUtf8();

        return SmallString(utf8ByteArray.constData(), uint(utf8ByteArray.size()));
    }

    static
    SmallString fromQByteArray(const QByteArray &utf8ByteArray)
    {
        return SmallString(utf8ByteArray.constData(), uint(utf8ByteArray.size()));
    }

    bool contains(SmallStringView subStringToSearch) const
    {
        auto found = std::search(begin(),
                                 end(),
                                 subStringToSearch.begin(),
                                 subStringToSearch.end());

        return found != end();
    }

    bool contains(char characterToSearch) const
    {
        auto found = std::strchr(data(), characterToSearch);

        return found != nullptr;
    }

    bool startsWith(SmallStringView subStringToSearch) const noexcept
    {
        if (size() >= subStringToSearch.size())
            return !std::memcmp(data(), subStringToSearch.data(), subStringToSearch.size());

        return false;
    }

    bool startsWith(char characterToSearch) const noexcept
    {
        return data()[0] == characterToSearch;
    }

    bool endsWith(SmallStringView subStringToSearch) const noexcept
    {
        if (size() >= subStringToSearch.size()) {
            const int comparison = std::memcmp(end().data() - subStringToSearch.size(),
                                               subStringToSearch.data(),
                                               subStringToSearch.size());
            return comparison == 0;
        }

        return false;
    }

    bool endsWith(char character) const noexcept
    {
        return at(size() - 1) == character;
    }

    size_type size() const noexcept
    {
        if (!isShortString())
            return m_data.allocated.data.size;

        return m_data.shortString.shortStringSize;
    }

    size_type capacity() const noexcept
    {
        if (!isShortString())
            return m_data.allocated.data.capacity;

        return shortStringCapacity();
    }

    bool isEmpty() const noexcept
    {
        return size() == 0;
    }

    bool hasContent() const noexcept
    {
        return size() != 0;
    }

    SmallStringView mid(size_type position) const noexcept
    {
        return SmallStringView(data() + position, size() - position);
    }

    SmallStringView mid(size_type position, size_type length) const noexcept
    {
        return SmallStringView(data() + position, length);
    }

    void append(SmallStringView string) noexcept
    {
        size_type oldSize = size();
        size_type newSize = oldSize + string.size();

        reserve(optimalCapacity(newSize));
        std::memcpy(data() + oldSize, string.data(), string.size());
        at(newSize) = 0;
        setSize(newSize);
    }

    void replace(SmallStringView fromText, SmallStringView toText)
    {
        if (toText.size() == fromText.size())
            replaceEqualSized(fromText, toText);
        else if (toText.size() < fromText.size())
            replaceSmallerSized(fromText, toText);
        else
            replaceLargerSized(fromText, toText);
    }

    void replace(size_type position, size_type length, SmallStringView replacementText)
    {
        size_type newSize = size() - length + replacementText.size();

        reserve(optimalCapacity(newSize));

        auto replaceStart = begin() + position;
        auto replaceEnd = replaceStart + length;
        auto replacementEnd = replaceStart + replacementText.size();
        size_type tailSize = size_type(end() - replaceEnd + 1);

        std::memmove(replacementEnd.data(),
                     replaceEnd.data(), tailSize);
        std::memcpy(replaceStart.data(), replacementText.data(), replacementText.size());

        setSize(newSize);
    }

    constexpr static
    size_type shortStringCapacity() noexcept
    {
        return SmallStringLiteral::shortStringCapacity();
    }

    size_type optimalCapacity(const size_type size)
    {
        if (fitsNotInCapacity(size))
           return optimalHeapCapacity(size + 1) - 1;

        return size;
    }

    size_type shortStringSize() const
    {
        return m_data.shortString.shortStringSize;
    }

    static
    SmallString join(std::initializer_list<SmallString> list)
    {
        size_type totalSize = 0;
        for (const SmallString &string : list)
            totalSize += string.size();

        SmallString joinedString;
        joinedString.reserve(totalSize);

        for (const SmallString &string : list)
            joinedString.append(string);

        return joinedString;
    }

UNIT_TEST_PUBLIC:
    bool isShortString() const noexcept
    {
        return !m_data.shortString.isReference;
    }

    bool isReadOnlyReference() const noexcept
    {
        return m_data.shortString.isReadOnlyReference;
    }

    bool hasAllocatedMemory() const noexcept
    {
        return !isShortString() && !isReadOnlyReference();
    }

    bool fitsNotInCapacity(size_type capacity) const noexcept
    {
        return (isShortString() && capacity > shortStringCapacity())
            || (!isShortString() && capacity > m_data.allocated.data.capacity);
    }

    static
    size_type optimalHeapCapacity(const size_type size)
    {
        const size_type cacheLineSize = 64;

        const auto divisionByCacheLineSize = std::div(int64_t(size), int64_t(cacheLineSize));

        size_type cacheLineBlocks = size_type(divisionByCacheLineSize.quot);
        const size_type supplement = divisionByCacheLineSize.rem ? 1 : 0;

        cacheLineBlocks += supplement;
        int exponent;
        const double significand = std::frexp(cacheLineBlocks, &exponent);
        const double factorOneDotFiveSignificant = std::ceil(significand * 4.) / 4.;
        cacheLineBlocks = size_type(std::ldexp(factorOneDotFiveSignificant, exponent));

        return cacheLineBlocks * cacheLineSize;
    }

private:
    constexpr
    SmallString(Internal::StringDataLayout data) noexcept
        : m_data(data)
    {
    }

    char &at(size_type index)
    {
        return *(data() + index);
    }

    const char &at(size_type index) const
    {
        return *(data() + index);
    }

    void replaceEqualSized(SmallStringView fromText, SmallStringView toText)
    {
        reserve(size());

        auto start = begin();
        auto found = std::search(start,
                                 end(),
                                 fromText.begin(),
                                 fromText.end());

        while (found != end()) {
            start = found + toText.size();

            std::memcpy(found.data(), toText.data(), toText.size());

            found = std::search(start,
                                     end(),
                                     fromText.begin(),
                                     fromText.end());

        }
    }

    void replaceSmallerSized(SmallStringView fromText, SmallStringView toText)
    {
        size_type newSize = size();
        reserve(newSize);

        auto start = begin();

        auto found = std::search(start,
                                 end(),
                                 fromText.begin(),
                                 fromText.end());

        size_type sizeDifference = 0;

        while (found != end()) {
            start = found + fromText.size();

            auto nextFound = std::search(start,
                                         end(),
                                         fromText.begin(),
                                         fromText.end());

            auto replacedTextEndPosition = found + fromText.size();
            auto replacementTextEndPosition = found + toText.size() - sizeDifference;
            auto replacementTextStartPosition = found - sizeDifference;

            std::memmove(replacementTextEndPosition.data(),
                         replacedTextEndPosition.data(),
                         nextFound - start);
            std::memcpy(replacementTextStartPosition.data(), toText.data(), toText.size());

            sizeDifference += fromText.size() - toText.size();
            found = nextFound;
        }
        newSize -= sizeDifference;
        setSize(newSize);
        *end() = 0;
    }

    iterator replaceLargerSizedRecursive(size_type startIndex,
                                         SmallStringView fromText,
                                         SmallStringView toText,
                                         size_type sizeDifference)
    {
        auto found = std::search(begin() + startIndex,
                                 end(),
                                 fromText.begin(),
                                 fromText.end());

        auto foundIndex = found - begin();

        if (found != end()) {
            startIndex = foundIndex + fromText.size();

            size_type newSizeDifference = sizeDifference + toText.size() - fromText.size();

            auto nextFound = replaceLargerSizedRecursive(startIndex,
                                                         fromText,
                                                         toText,
                                                         newSizeDifference);

            found = begin() + foundIndex;
            auto start = begin() + startIndex;

            auto replacedTextEndPosition = found + fromText.size();
            auto replacementTextEndPosition = found + fromText.size() + newSizeDifference;
            auto replacementTextStartPosition = found + sizeDifference;


            std::memmove(replacementTextEndPosition.data(),
                         replacedTextEndPosition.data(),
                         nextFound - start);
            std::memcpy(replacementTextStartPosition.data(), toText.data(), toText.size());
        } else {
            size_type newSize = size() + sizeDifference;
            reserve(optimalCapacity(newSize));
            setSize(newSize);
            found = end();
            *end() = 0;
        }

        return found;
    }

    void replaceLargerSized(SmallStringView fromText, SmallStringView toText)
    {
        size_type sizeDifference = 0;
        size_type startIndex = 0;

        replaceLargerSizedRecursive(startIndex, fromText, toText, sizeDifference);
    }

    void setSize(size_type size)
    {
        if (isShortString())
            m_data.shortString.shortStringSize = uchar(size);
        else
            m_data.allocated.data.size = size;
    }

private:
    Internal::StringDataLayout m_data;
};

template<SmallString::size_type Size>
bool operator==(const SmallString& first, const char(&second)[Size]) noexcept
{
    return first == SmallStringView(second);
}

template<SmallString::size_type Size>
bool operator==(const char(&first)[Size], const SmallString& second) noexcept
{
    return second == first;
}

template<typename Type,
         typename = typename std::enable_if<std::is_pointer<Type>::value>::type
         >
bool operator==(const SmallString& first, Type second) noexcept
{
    return first == SmallStringView(second);
}

template<typename Type,
         typename = typename std::enable_if<std::is_pointer<Type>::value>::type
         >
bool operator==(Type first, const SmallString& second) noexcept
{
    return second == first;
}

inline
bool operator==(const SmallString& first, const SmallString& second) noexcept
{
    return first.size() == second.size()
        && std::memcmp(first.data(), second.data(), first.size()) == 0;
}

inline
bool operator==(const SmallString& first, const SmallStringView& second) noexcept
{
    return first.size() == second.size()
        && std::memcmp(first.data(), second.data(), first.size()) == 0;
}

inline
bool operator==(const SmallStringView& first, const SmallString& second) noexcept
{
    return second == first;
}

inline
bool operator!=(const SmallString& first, const SmallStringView& second) noexcept
{
    return !(first == second);
}

inline
bool operator!=(const SmallStringView& first, const SmallString& second) noexcept
{
    return second == first;
}

inline
bool operator!=(const SmallString& first, const SmallString& second) noexcept
{
    return !(first == second);
}

template<SmallString::size_type Size>
bool operator!=(const SmallString& first, const char(&second)[Size]) noexcept
{
    return !(first == second);
}

template<SmallString::size_type Size>
bool operator!=(const char(&first)[Size], const SmallString& second) noexcept
{
    return second != first;
}

template<typename Type,
         typename = typename std::enable_if<std::is_pointer<Type>::value>::type
         >
bool operator!=(const SmallString& first, Type second) noexcept
{
    return !(first == second);
}

template<typename Type,
         typename = typename std::enable_if<std::is_pointer<Type>::value>::type
         >
bool operator!=(Type first, const SmallString& second) noexcept
{
    return second != first;
}

inline
bool operator<(const SmallString& first, const SmallString& second) noexcept
{
    SmallString::size_type minimalSize = std::min(first.size(), second.size());

    const int comparison = std::memcmp(first.data(), second.data(), minimalSize + 1);

    return comparison < 0;
}

inline
QDataStream &operator<<(QDataStream &out, const SmallString &string)
{
   if (string.isEmpty())
       out << quint32(0);
   else
       out.writeBytes(string.data(), qint32(string.size()));

   return out;
}

inline
QDataStream &operator>>(QDataStream &in, SmallString &string)
{
    quint32 size;

    in >> size;

    if (size > 0 ) {
        string.resize(size);

        char *data = string.data();

        in.readRawData(data, size);
    }

    return in;
}

inline
QDebug &operator<<(QDebug &debug, const SmallString &string)
{
    debug.nospace() << "\"" << string.data() << "\"";

    return debug;
}

inline
std::ostream &operator<<(std::ostream &stream, const SmallString &string)
{
    return stream << string.data();
}

inline
void PrintTo(const SmallString &string, ::std::ostream *os)
{
    *os << "'" << string.data() << "'";
}

} // namespace Utils

namespace std {

template<> struct hash<Utils::SmallString>
{
    using argument_type = Utils::SmallString;
    using result_type = uint;
    result_type operator()(const argument_type& string) const
    {
        return qHashBits(string.data(), string.size());
    }
};

} // namespace std

#pragma pop_macro("noexcept")
#pragma pop_macro("constexpr")
