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
#include <QString>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <climits>
#include <cstring>
#include <initializer_list>
#include <numeric>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#ifdef UNIT_TESTS
#define unittest_public public
#else
#define unittest_public private
#endif

namespace Utils {

template<uint Size>
class BasicSmallString
{
public:
    using iterator = Internal::SmallStringIterator<std::random_access_iterator_tag, char>;
    using const_iterator = Internal::SmallStringIterator<std::random_access_iterator_tag, const char>;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;
    using size_type = std::size_t;

    static_assert(Size < 64
                ? sizeof(Internal::StringDataLayout<Size>) == Size + 1
                : sizeof(Internal::StringDataLayout<Size>) == Size + 2,
                  "Size is wrong");

    BasicSmallString() noexcept
        : m_data(Internal::StringDataLayout<Size>())
    {
    }

    constexpr
    BasicSmallString(const BasicSmallStringLiteral<Size> &stringReference)
        : m_data(stringReference.m_data)
    {
    }

    template<size_type ArraySize>
    constexpr
    BasicSmallString(const char(&string)[ArraySize])
        : m_data(string)
    {
    }

    BasicSmallString(const char *string, size_type size, size_type capacity)
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
            initializeLongString(size, capacity);
        }
    }

    explicit BasicSmallString(SmallStringView stringView)
        : BasicSmallString(stringView.data(), stringView.size(), stringView.size())
    {
    }

    BasicSmallString(const char *string, size_type size)
        : BasicSmallString(string, size, size)
    {
    }

    template<typename Type,
             typename = std::enable_if_t<std::is_pointer<Type>::value>
             >
    BasicSmallString(Type characterPointer)
        : BasicSmallString(characterPointer, std::strlen(characterPointer))
    {
        static_assert(!std::is_array<Type>::value, "Input type is array and not char pointer!");
    }

    BasicSmallString(const QString &qString)
        : BasicSmallString(BasicSmallString::fromQString(qString))
    {}

    template<typename Type,
             typename = std::enable_if_t<std::is_same<std::decay_t<Type>, std::string>::value>
             >
    BasicSmallString(Type &&string)
        : BasicSmallString(string.data(), string.size())
    {}

    template<typename BeginIterator,
             typename EndIterator,
             typename = std::enable_if_t<std::is_same<BeginIterator, EndIterator>::value>
             >
    BasicSmallString(BeginIterator begin, EndIterator end)
        : BasicSmallString(&(*begin), size_type(end - begin))
    {
    }

    BasicSmallString(std::initializer_list<Utils::SmallStringView> list)
    {
        std::size_t size = std::accumulate(list.begin(),
                                           list.end(),
                                           std::size_t(0),
                                           [] (std::size_t size, Utils::SmallStringView string) {
                return size + string.size();
         });

        reserve(size);
        setSize(size);

        char *currentData = data();

        for (Utils::SmallStringView string : list) {
            std::memcpy(currentData, string.data(), string.size());

            currentData += string.size();
        }

        at(size) = 0;
    }

    ~BasicSmallString() noexcept
    {
        if (Q_UNLIKELY(hasAllocatedMemory()))
            Memory::deallocate(m_data.allocated.data.pointer);
    }

    BasicSmallString(const BasicSmallString &string)
    {
        if (string.isShortString() || string.isReadOnlyReference())
            m_data = string.m_data;
        else
            new (this) BasicSmallString{string.data(), string.size()};
    }

    BasicSmallString &operator=(const BasicSmallString &other)
    {
        BasicSmallString copy = other;

        swap(*this, copy);

        return *this;
    }

    BasicSmallString(BasicSmallString &&other) noexcept
    {
        m_data = other.m_data;
        other.m_data = Internal::StringDataLayout<Size>();
    }

    BasicSmallString &operator=(BasicSmallString &&other) noexcept
    {
        swap(*this, other);

        return *this;
    }

    BasicSmallString clone() const
    {
        BasicSmallString clonedString(m_data);

        if (Q_UNLIKELY(hasAllocatedMemory()))
            new (&clonedString) BasicSmallString{m_data.allocated.data.pointer, m_data.allocated.data.size};

        return clonedString;
    }

    friend
    void swap(BasicSmallString &first, BasicSmallString &second)
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

    operator std::string() const
    {
        return std::string(data(), size());
    }

    static
    BasicSmallString fromUtf8(const char *characterPointer)
    {
        return BasicSmallString(characterPointer, std::strlen(characterPointer));
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
                newCapacity = std::max(newCapacity, oldSize);
                const char *oldData = data();

                char *newData = Memory::allocate(newCapacity + 1);
                std::memcpy(newData, oldData, oldSize);
                m_data.allocated.data.pointer = newData;
                initializeLongString(oldSize, newCapacity);
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
        this->~BasicSmallString();
        m_data = Internal::StringDataLayout<Size>();
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
        return reverse_iterator(end() - static_cast<std::size_t>(1));
    }

    reverse_iterator rend() noexcept
    {
        return reverse_iterator(begin() - static_cast<std::size_t>(1));
    }

    const_reverse_iterator rbegin() const noexcept
    {
        return const_reverse_iterator(end() - static_cast<std::size_t>(1));
    }

    const_reverse_iterator rend() const noexcept
    {
        return const_reverse_iterator(begin() - static_cast<std::size_t>(1));
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
    BasicSmallString fromQString(const QString &qString)
    {
        const QByteArray &utf8ByteArray = qString.toUtf8();

        return BasicSmallString(utf8ByteArray.constData(), uint(utf8ByteArray.size()));
    }

    static
    BasicSmallString fromQByteArray(const QByteArray &utf8ByteArray)
    {
        return BasicSmallString(utf8ByteArray.constData(), uint(utf8ByteArray.size()));
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

    bool empty() const noexcept
    {
        return isEmpty();
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

    BasicSmallString &operator+=(SmallStringView string)
    {
        append(string);

        return *this;
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

    void replace(char fromCharacter, char toCharacter)
    {
        reserve(size());

        auto operation = [=] (char currentCharacter) {
            return currentCharacter == fromCharacter ? toCharacter : currentCharacter;
        };

        std::transform(begin(), end(), begin(), operation);
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

    BasicSmallString toCarriageReturnsStripped() const
    {
        BasicSmallString text = *this;

        text.replace("\r", "");

        return text;
    }

    constexpr static
    size_type shortStringCapacity() noexcept
    {
        return Internal::StringDataLayout<Size>::shortStringCapacity();
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
    BasicSmallString join(std::initializer_list<BasicSmallString> list)
    {
        size_type totalSize = 0;
        for (const BasicSmallString &string : list)
            totalSize += string.size();

        BasicSmallString joinedString;
        joinedString.reserve(totalSize);

        for (const BasicSmallString &string : list)
            joinedString.append(string);

        return joinedString;
    }

    char &operator[](std::size_t index)
    {
        return *(data() + index);
    }

    char operator[](std::size_t index) const
    {
        return *(data() + index);
    }

    template<size_type ArraySize>
    friend bool operator==(const BasicSmallString& first, const char(&second)[ArraySize]) noexcept
    {
        return first == SmallStringView(second);
    }

    template<size_type ArraySize>
    friend bool operator==(const char(&first)[ArraySize], const BasicSmallString& second) noexcept
    {
        return second == first;
    }

    template<typename Type,
             typename = std::enable_if_t<std::is_pointer<Type>::value>
             >
    friend bool operator==(const BasicSmallString& first, Type second) noexcept
    {
        return first == SmallStringView(second);
    }

    template<typename Type,
             typename = std::enable_if_t<std::is_pointer<Type>::value>
             >
    friend bool operator==(Type first, const BasicSmallString& second) noexcept
    {
        return second == first;
    }

    friend bool operator==(const BasicSmallString& first, const SmallStringView& second) noexcept
    {
        return first.size() == second.size()
            && std::memcmp(first.data(), second.data(), first.size()) == 0;
    }

    friend bool operator==(const SmallStringView& first, const BasicSmallString& second) noexcept
    {
        return second == first;
    }

    friend bool operator!=(const BasicSmallString& first, const SmallStringView& second) noexcept
    {
        return !(first == second);
    }

    friend bool operator!=(const SmallStringView& first, const BasicSmallString& second) noexcept
    {
        return second == first;
    }

    friend bool operator!=(const BasicSmallString& first, const BasicSmallString& second) noexcept
    {
        return !(first == second);
    }

    template<size_type ArraySize>
    friend bool operator!=(const BasicSmallString& first, const char(&second)[ArraySize]) noexcept
    {
        return !(first == second);
    }

    template<size_type ArraySize>
    friend bool operator!=(const char(&first)[ArraySize], const BasicSmallString& second) noexcept
    {
        return second != first;
    }

    template<typename Type,
             typename = std::enable_if_t<std::is_pointer<Type>::value>
             >
    friend bool operator!=(const BasicSmallString& first, Type second) noexcept
    {
        return !(first == second);
    }

    template<typename Type,
             typename = std::enable_if_t<std::is_pointer<Type>::value>
             >
    friend bool operator!=(Type first, const BasicSmallString& second) noexcept
    {
        return second != first;
    }

    friend bool operator<(const BasicSmallString& first, SmallStringView second) noexcept
    {
        if (first.size() != second.size())
            return first.size() < second.size();

        const int comparison = std::memcmp(first.data(), second.data(), first.size());

        return comparison < 0;
    }

    friend bool operator<(SmallStringView first, const BasicSmallString& second) noexcept
    {
        if (first.size() != second.size())
            return first.size() < second.size();

        const int comparison = std::memcmp(first.data(), second.data(), first.size());

        return comparison < 0;
    }

unittest_public:
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
    BasicSmallString(Internal::StringDataLayout<Size> data) noexcept
        : m_data(data)
    {
    }

    void initializeLongString(size_type size, size_type capacity)
    {
        m_data.allocated.data.pointer[size] = 0;
        m_data.allocated.data.size = size;
        m_data.allocated.data.capacity = capacity;
        m_data.allocated.shortStringSize = 0;
        m_data.allocated.isReference = true;
        m_data.allocated.isReadOnlyReference = false;
    }

    char &at(size_type index)
    {
        return *(data() + index);
    }

    char at(size_type index) const
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
    Internal::StringDataLayout<Size> m_data;
};

template<template<uint> class String, uint Size>
using isSameString = std::is_same<std::remove_reference_t<std::remove_cv_t<String<Size>>>,
                                           BasicSmallString<Size>>;

template<template<uint> class String,
         uint SizeOne,
         uint SizeTwo,
         typename =  std::enable_if_t<isSameString<String, SizeOne>::value
                                   || isSameString<String, SizeTwo>::value>>
bool operator==(const String<SizeOne> &first, const String<SizeTwo> &second) noexcept
{
    return first.size() == second.size()
        && std::memcmp(first.data(), second.data(), first.size()) == 0;
}


template<template<uint> class String,
         uint SizeOne,
         uint SizeTwo,
         typename =  std::enable_if_t<isSameString<String, SizeOne>::value
                                   || isSameString<String, SizeTwo>::value>>
bool operator<(const String<SizeOne> &first, const String<SizeTwo> &second) noexcept
{
    if (first.size() != second.size())
        return first.size() < second.size();

    const int comparison = std::memcmp(first.data(), second.data(), first.size() + 1);

    return comparison < 0;
}

template<typename Key,
         typename Value,
         typename Hash = std::hash<Key>,
         typename KeyEqual = std::equal_to<Key>,
         typename Allocator = std::allocator<std::pair<const Key, Value>>>
std::unordered_map<Key, Value, Hash, KeyEqual, Allocator>
clone(const std::unordered_map<Key, Value, Hash, KeyEqual, Allocator> &map)
{
    std::unordered_map<Key, Value, Hash, KeyEqual, Allocator> clonedMap;
    clonedMap.reserve(clonedMap.size());

    for (auto &&entry : map)
        clonedMap.emplace(entry.first, entry.second.clone());

    return clonedMap;
}

template <typename Type>
std::vector<Type> clone(const std::vector<Type> &vector)
{
    std::vector<Type> clonedVector;
    clonedVector.reserve(vector.size());

    for (auto &&entry : vector)
        clonedVector.push_back(entry.clone());

    return clonedVector;
}

using SmallString = BasicSmallString<31>;
using PathString = BasicSmallString<190>;

} // namespace Utils
