// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include "smallstringliteral.h"
#include "smallstringiterator.h"
#include "smallstringview.h"
#include "smallstringmemory.h"

#include <QByteArray>
#include <QString>
#include <QStringEncoder>

#include <algorithm>
#include <charconv>
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

#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif

namespace Utils {

template<uint Size>
class alignas(16) BasicSmallString
{
public:
    using const_iterator = Internal::SmallStringIterator<std::random_access_iterator_tag, const char>;
    using iterator = Internal::SmallStringIterator<std::random_access_iterator_tag, char>;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;
    using size_type = std::size_t;

    static_assert(Size < 64
                ? sizeof(Internal::StringDataLayout<Size>) == Size + 1
                : sizeof(Internal::StringDataLayout<Size>) == Size + 2,
                  "Size is wrong");
    constexpr
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
    {}

    BasicSmallString(const char *string, size_type size, size_type capacity)
    {
        if (Q_LIKELY(capacity <= shortStringCapacity())) {
            m_data.control = Internal::ControlBlock<Size>(size, false, false);
            std::char_traits<char>::copy(m_data.shortString, string, size);
        } else {
            auto pointer = Memory::allocate(capacity);
            std::char_traits<char>::copy(pointer, string, size);
            initializeLongString(pointer, size, capacity);
        }
    }

    explicit BasicSmallString(SmallStringView stringView)
        : BasicSmallString(stringView.data(), stringView.size(), stringView.size())
    {}

    BasicSmallString(const char *string, size_type size)
        : BasicSmallString(string, size, size)
    {}

    explicit BasicSmallString(const_iterator begin, const_iterator end)
        : BasicSmallString{std::addressof(*begin), static_cast<std::size_t>(std::distance(begin, end))}
    {}

    explicit BasicSmallString(iterator begin, iterator end)

        : BasicSmallString{std::addressof(*begin), static_cast<std::size_t>(std::distance(begin, end))}
    {}

    template<typename Type, typename = std::enable_if_t<std::is_pointer<Type>::value>>
    BasicSmallString(Type characterPointer)
        : BasicSmallString(characterPointer, std::char_traits<char>::length(characterPointer))
    {
        static_assert(!std::is_array<Type>::value, "Input type is array and not char pointer!");
    }

    BasicSmallString(const QString &qString)
        : BasicSmallString(BasicSmallString::fromQString(qString))
    {}

    BasicSmallString(const QStringView qStringView)
        : BasicSmallString(BasicSmallString::fromQStringView(qStringView))
    {}

    BasicSmallString(const QByteArray &qByteArray)
        : BasicSmallString(qByteArray.constData(), qByteArray.size())
    {}

    template<typename String,
             typename Utils::enable_if_has_char_data_pointer<String> = 0>
    BasicSmallString(const String &string)
        : BasicSmallString(string.data(), string.size())
    {
    }

    BasicSmallString(const std::wstring &wstring)
        : BasicSmallString(BasicSmallString::fromQStringView(wstring))
    {}

    template<typename BeginIterator,
             typename EndIterator,
             typename = std::enable_if_t<std::is_same<BeginIterator, EndIterator>::value>
             >
    BasicSmallString(BeginIterator begin, EndIterator end)
        : BasicSmallString(&(*begin), size_type(end - begin))
    {}

    ~BasicSmallString() noexcept
    {
        if (Q_UNLIKELY(hasAllocatedMemory()))
            Memory::deallocate(m_data.reference.pointer);
    }

    BasicSmallString(const BasicSmallString &other)
    {
        if (Q_LIKELY(other.isShortString() || other.isReadOnlyReference()))
            m_data = other.m_data;
        else
            new (this) BasicSmallString{other.data(), other.size()};
    }

    BasicSmallString &operator=(const BasicSmallString &other)
    {
        if (Q_LIKELY(this != &other)) {
            this->~BasicSmallString();

            if (Q_LIKELY(other.isShortString() || other.isReadOnlyReference()))
                m_data = other.m_data;
            else
                new (this) BasicSmallString{other.data(), other.size()};
        }

        return *this;
    }

    BasicSmallString(BasicSmallString &&other) noexcept
        : m_data(std::move(other.m_data))
    {
        other.m_data.reset();
    }

    BasicSmallString &operator=(BasicSmallString &&other) noexcept
    {
        if (Q_LIKELY(this != &other)) {
            this->~BasicSmallString();

            m_data = std::move(other.m_data);
            other.m_data.reset();
        }

        return *this;
    }

    BasicSmallString take() noexcept { return std::move(*this); }
    BasicSmallString clone() const
    {
        BasicSmallString clonedString(m_data);

        if (Q_UNLIKELY(hasAllocatedMemory()))
            new (&clonedString) BasicSmallString{m_data.reference.pointer,
                                                 m_data.reference.size,
                                                 m_data.reference.capacity};

        return clonedString;
    }

    friend void swap(BasicSmallString &first, BasicSmallString &second) noexcept
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

    SmallStringView toStringView() const
    {
        return SmallStringView(data(), size());
    }

    operator SmallStringView() const
    {
        return SmallStringView(data(), size());
    }

    explicit
    operator QString() const
    {
        return toQString();
    }

    explicit
    operator std::string() const
    {
        return std::string(data(), size());
    }

    static
    BasicSmallString fromUtf8(const char *characterPointer)
    {
        return BasicSmallString(characterPointer, std::char_traits<char>::length(characterPointer));
    }

    void reserve(size_type newCapacity)
    {
        if (fitsNotInCapacity(newCapacity)) {
            if (Q_UNLIKELY(hasAllocatedMemory())) {
                m_data.reference.pointer = Memory::reallocate(m_data.reference.pointer, newCapacity);
                m_data.reference.capacity = newCapacity;
            } else {
                auto oldSize = size();
                new (this) BasicSmallString{data(), oldSize, std::max(newCapacity, oldSize)};
            }
        }
    }

    void resize(size_type newSize)
    {
        reserve(newSize);
        setSize(newSize);
    }

    void clear() noexcept
    {
        this->~BasicSmallString();
        m_data = Internal::StringDataLayout<Size>();
    }

    char *data() noexcept
    {
        return Q_LIKELY(isShortString()) ? m_data.shortString : m_data.reference.pointer;
    }

    const char *data() const noexcept
    {
        return Q_LIKELY(isShortString()) ? m_data.shortString : m_data.reference.pointer;
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
        return reverse_iterator(end());
    }

    reverse_iterator rend() noexcept
    {
        return reverse_iterator(begin());
    }

    const_reverse_iterator rbegin() const noexcept
    {
        return const_reverse_iterator(end());
    }

    const_reverse_iterator rend() const noexcept
    {
        return const_reverse_iterator(begin());
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

    static BasicSmallString fromQString(const QString &qString)
    {
        BasicSmallString string;
        string.append(qString);

        return string;
    }

    static BasicSmallString fromQStringView(QStringView qStringView)
    {
        BasicSmallString string;
        string.append(qStringView);

        return string;
    }

    static BasicSmallString fromQByteArray(const QByteArray &utf8ByteArray)
    {
        return BasicSmallString(utf8ByteArray.constData(), uint(utf8ByteArray.size()));
    }

    // precondition: has to be null terminated
    bool contains(SmallStringView subStringToSearch) const
    {
        const char *found = std::strstr(data(), subStringToSearch.data());

        return found != nullptr;
    }

    bool contains(char characterToSearch) const
    {
        auto found = std::char_traits<char>::find(data(), size(), characterToSearch);

        return found != nullptr;
    }

    bool startsWith(SmallStringView subStringToSearch) const noexcept
    {
        if (size() >= subStringToSearch.size())
            return !std::char_traits<char>::compare(data(),
                                                    subStringToSearch.data(),
                                                    subStringToSearch.size());

        return false;
    }

    bool startsWith(char characterToSearch) const noexcept
    {
        return data()[0] == characterToSearch;
    }

    bool endsWith(SmallStringView subStringToSearch) const noexcept
    {
        if (size() >= subStringToSearch.size()) {
            const int comparison = std::char_traits<char>::compare(end().data()
                                                                       - subStringToSearch.size(),
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
            return m_data.reference.size;

        return m_data.control.shortStringSize();
    }

    size_type capacity() const noexcept
    {
        if (!isShortString())
            return m_data.reference.capacity;

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

    void append(SmallStringView string)
    {
        size_type oldSize = size();
        size_type newSize = oldSize + string.size();

        reserve(optimalCapacity(newSize));
        std::char_traits<char>::copy(data() + oldSize, string.data(), string.size());
        setSize(newSize);
    }

    void append(QStringView string)
    {
        QStringEncoder encoder{QStringEncoder::Utf8};

        constexpr size_type temporaryArraySize = Size * 6;

        size_type oldSize = size();
        size_type maximumRequiredSize = static_cast<size_type>(encoder.requiredSpace(oldSize));
        char *newEnd = nullptr;

        if (maximumRequiredSize > temporaryArraySize) {
            size_type newSize = oldSize + maximumRequiredSize;

            reserve(optimalCapacity(newSize));
            newEnd = encoder.appendToBuffer(data() + oldSize, string);
        } else {
            char temporaryArray[temporaryArraySize];

            auto newTemporaryArrayEnd = encoder.appendToBuffer(temporaryArray, string);

            auto newAppendedStringSize = newTemporaryArrayEnd - temporaryArray;
            size_type newSize = oldSize + newAppendedStringSize;

            reserve(optimalCapacity(newSize));

            std::memcpy(data() + oldSize, temporaryArray, newAppendedStringSize);

            newEnd = data() + newSize;
        }
        setSize(newEnd - data());
    }

    BasicSmallString &operator+=(SmallStringView string)
    {
        append(string);

        return *this;
    }

    BasicSmallString &operator+=(QStringView string)
    {
        append(string);

        return *this;
    }

    BasicSmallString &operator+=(std::initializer_list<SmallStringView> list)
    {
        appendInitializerList(list, size());

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

        std::replace(begin(), end(), fromCharacter, toCharacter);
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
            return optimalHeapCapacity(size);

        return size;
    }

    constexpr size_type shortStringSize() const { return m_data.control.shortStringSize(); }

    static
    BasicSmallString join(std::initializer_list<SmallStringView> list)
    {
        size_type totalSize = 0;
        for (SmallStringView string : list)
            totalSize += string.size();

        BasicSmallString joinedString;
        joinedString.reserve(totalSize);

        for (SmallStringView string : list)
            joinedString.append(string);

        return joinedString;
    }

    static
    BasicSmallString number(int number)
    {
#ifdef __cpp_lib_to_chars
        // +1 for the sign, +1 for the extra digit
        char buffer[std::numeric_limits<int>::digits10 + 2];
        auto result = std::to_chars(buffer, buffer + sizeof(buffer), number, 10);
        Q_ASSERT(result.ec == std::errc{});
        auto endOfConversionString = result.ptr;
        return BasicSmallString(buffer, endOfConversionString);
#else
        return std::to_string(number);
#endif
    }

    static
    BasicSmallString number(long long int number)
    {
#ifdef __cpp_lib_to_chars
        // +1 for the sign, +1 for the extra digit
        char buffer[std::numeric_limits<long long int>::digits10 + 2];
        auto result = std::to_chars(buffer, buffer + sizeof(buffer), number, 10);
        Q_ASSERT(result.ec == std::errc{});
        auto endOfConversionString = result.ptr;
        return BasicSmallString(buffer, endOfConversionString);
#else
        return std::to_string(number);
#endif
    }

    static
    BasicSmallString number(double number)
    {
        return std::to_string(number);
    }

    char &operator[](std::size_t index)
    {
        return *(data() + index);
    }

    char operator[](std::size_t index) const
    {
        return *(data() + index);
    }

    friend BasicSmallString operator+(const BasicSmallString &first, const BasicSmallString &second)
    {
        BasicSmallString text;
        text.reserve(first.size() + second.size());

        text.append(first);
        text.append(second);

        return text;
    }

    friend BasicSmallString operator+(const BasicSmallString &first, SmallStringView second)
    {
        BasicSmallString text;
        text.reserve(first.size() + second.size());

        text.append(first);
        text.append(second);

        return text;
    }

    friend BasicSmallString operator+(SmallStringView first, const BasicSmallString &second)
    {
        BasicSmallString text;
        text.reserve(first.size() + second.size());

        text.append(first);
        text.append(second);

        return text;
    }

    template<size_type ArraySize>
    friend BasicSmallString operator+(const BasicSmallString &first, const char(&second)[ArraySize])
    {

        return operator+(first, SmallStringView(second));
    }

    template<size_type ArraySize>
    friend BasicSmallString operator+(const char(&first)[ArraySize], const BasicSmallString &second)
    {
        return operator+(SmallStringView(first), second);
    }

unittest_public:
    constexpr
    bool isShortString() const noexcept
    {
        return m_data.control.isShortString();
    }

    constexpr
    bool isReadOnlyReference() const noexcept
    {
        return m_data.control.isReadOnlyReference();
    }

    constexpr
    bool hasAllocatedMemory() const noexcept
    {
        return !isShortString() && !isReadOnlyReference();
    }

    bool fitsNotInCapacity(size_type capacity) const noexcept
    {
        return (isShortString() && capacity > shortStringCapacity())
               || (!isShortString() && capacity > m_data.reference.capacity);
    }

    static
    size_type optimalHeapCapacity(const size_type size)
    {
        const size_type cacheLineSize = 64;

        size_type cacheLineBlocks = (size - 1) / cacheLineSize;

        return (cacheLineBlocks  + 1) * cacheLineSize;
    }

    size_type countOccurrence(SmallStringView text)
    {
        auto found = begin();

        size_type count = 0;

        while (true) {
            found = std::search(found,
                                end(),
                                text.begin(),
                                text.end());
            if (found == end())
                break;

            ++count;
            found += text.size();
        }

        return count;
    }

private:
    BasicSmallString(const Internal::StringDataLayout<Size> &data) noexcept
        : m_data(data)
    {
    }

    void appendInitializerList(std::initializer_list<SmallStringView> list, std::size_t initialSize)
    {
        auto addSize =  [] (std::size_t size, SmallStringView string) {
            return size + string.size();
        };

        std::size_t size = std::accumulate(list.begin(), list.end(), initialSize, addSize);

        reserve(size);
        setSize(size);

        char *currentData = data() + initialSize;

        for (SmallStringView string : list) {
            std::memcpy(currentData, string.data(), string.size());
            currentData += string.size();
        }
    }

    constexpr void initializeLongString(char *pointer, size_type size, size_type capacity)
    {
        m_data.control = Internal::ControlBlock<Size>(0, false, true);
        m_data.reference.pointer = pointer;
        m_data.reference.size = size;
        m_data.reference.capacity = capacity;
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

            std::char_traits<char>::copy(found.data(), toText.data(), toText.size());

            found = std::search(start,
                                     end(),
                                     fromText.begin(),
                                     fromText.end());

        }
    }

    void replaceSmallerSized(SmallStringView fromText, SmallStringView toText)
    {
        auto found = std::search(begin(),
                                 end(),
                                 fromText.begin(),
                                 fromText.end());

        if (found != end()) {
            size_type newSize = size();
            {
                size_type foundIndex = found - begin();
                reserve(newSize);
                found = begin() + foundIndex;
            }
            size_type sizeDifference = 0;

            while (found != end()) {
                auto start = found + fromText.size();

                auto nextFound = std::search(start,
                                             end(),
                                             fromText.begin(),
                                             fromText.end());

                auto replacedTextEndPosition = found + fromText.size();
                auto replacementTextEndPosition = found + toText.size() - sizeDifference;
                auto replacementTextStartPosition = found - sizeDifference;
                std::memmove(replacementTextEndPosition.data(),
                             replacedTextEndPosition.data(),
                             std::distance(start, nextFound));
                std::memcpy(replacementTextStartPosition.data(), toText.data(), toText.size());

                sizeDifference += fromText.size() - toText.size();
                found = nextFound;
            }

            newSize -= sizeDifference;
            setSize(newSize);
        }
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
            size_type startNextSearchIndex = foundIndex + fromText.size();
            size_type newSizeDifference = sizeDifference + (toText.size() - fromText.size());

            auto nextFound = replaceLargerSizedRecursive(startNextSearchIndex,
                                                         fromText,
                                                         toText,
                                                         newSizeDifference);

            auto startFound = begin() + foundIndex;
            auto endOfFound = begin() + startNextSearchIndex;

            auto replacedTextEndPosition = endOfFound;
            auto replacementTextEndPosition = endOfFound + newSizeDifference;
            auto replacementTextStartPosition = startFound + sizeDifference;

            std::memmove(replacementTextEndPosition.data(),
                         replacedTextEndPosition.data(),
                         std::distance(endOfFound, nextFound));
            std::memcpy(replacementTextStartPosition.data(), toText.data(), toText.size());
        } else if (startIndex != 0) {
            size_type newSize = size() + sizeDifference;
            setSize(newSize);
        }

        return begin() + foundIndex;
    }

    void replaceLargerSized(SmallStringView fromText, SmallStringView toText)
    {
        size_type sizeDifference = 0;
        size_type startIndex = 0;

        size_type replacementTextSizeDifference = toText.size() - fromText.size();
        size_type occurrences = countOccurrence(fromText);
        size_type newSize = size() + (replacementTextSizeDifference * occurrences);

        if (occurrences > 0) {
            reserve(optimalCapacity(newSize));

            replaceLargerSizedRecursive(startIndex, fromText, toText, sizeDifference);
        }
    }

    void setSize(size_type size)
    {
        if (isShortString())
            m_data.control.setShortStringSize(size);
        else
            m_data.reference.size = size;
    }

private:
    Internal::StringDataLayout<Size> m_data;
};

template<template<uint> class String, uint Size>
using isSameString = std::is_same<std::remove_reference_t<std::remove_cv_t<String<Size>>>,
                                           BasicSmallString<Size>>;

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

template<typename Type>
Type clone(const Type &vector)
{
    return vector;
}

using SmallString = BasicSmallString<31>;
using PathString = BasicSmallString<190>;

inline
SmallString operator+(SmallStringView first, SmallStringView second)
{
    SmallString text;
    text.reserve(first.size() + second.size());

    text.append(first);
    text.append(second);

    return text;
}

template<std::size_t Size>
inline
SmallString operator+(SmallStringView first, const char(&second)[Size])
{
    return operator+(first, SmallStringView(second));
}

template<std::size_t Size>
inline
SmallString operator+(const char(&first)[Size], SmallStringView second)
{
    return operator+(SmallStringView(first), second);
}

} // namespace Utils

#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic pop
#endif
