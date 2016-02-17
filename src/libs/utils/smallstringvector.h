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

#include "smallstring.h"

#include <vector>

#pragma push_macro("noexcept")
#ifndef __cpp_noexcept
#define noexcept
#endif

namespace Utils {

class SmallStringVector : public std::vector<Utils::SmallString>
{
public:
    SmallStringVector() = default;

    SmallStringVector(std::initializer_list<Utils::SmallString> list)
    {
        reserve(list.size());

        for (auto &&entry : list)
            push_back(entry.clone());
    }

    explicit SmallStringVector(const QStringList &stringList)
    {
        reserve(std::size_t(stringList.count()));

        for (const QString &string : stringList)
            push_back(Utils::SmallString::fromQString(string));
    }

#if !defined(UNIT_TESTS) && !(defined(_MSC_VER) && _MSC_VER < 1900)
    SmallStringVector(const SmallStringVector &) = delete;
    SmallStringVector &operator=(const SmallStringVector &) = delete;
#else
    SmallStringVector(const SmallStringVector &) = default;
    SmallStringVector &operator=(const SmallStringVector &) = default;
#endif

#if !(defined(_MSC_VER) && _MSC_VER < 1900)
    SmallStringVector(SmallStringVector &&) noexcept = default;
    SmallStringVector &operator=(SmallStringVector &&) noexcept = default;
#else
    SmallStringVector(SmallStringVector &&other)
        : std::vector<Utils::SmallString>(std::move(other))
    {
    }

    SmallStringVector &operator=(SmallStringVector &&other)
    {
        std::vector<Utils::SmallString>(std::move(other));

        return *this;
    }
#endif

    Utils::SmallString join(Utils::SmallString &&separator) const
    {
        Utils::SmallString joinedString;

        joinedString.reserve(totalByteSize() + separator.size() * std::size_t(size()));

        for (auto stringIterator = begin(); stringIterator != end(); ++stringIterator) {
            joinedString.append(*stringIterator);
            if (std::next(stringIterator) != end())
                joinedString.append(separator);
        }

        return joinedString;
    }

    bool contains(const Utils::SmallString &string) const noexcept
    {
        return std::find(cbegin(), cend(), string) != cend();
    }

    bool removeFast(Utils::SmallStringView valueToBeRemoved)
    {
        auto position = std::remove(begin(), end(), valueToBeRemoved);

        const bool hasEntry = position != end();

        erase(position, end());

        return hasEntry;
    }

    void append(Utils::SmallString &&string)
    {
        push_back(std::move(string));
    }

    SmallStringVector clone() const
    {
        SmallStringVector clonedVector;
        clonedVector.reserve(size());

        for (auto &&entry : *this)
            clonedVector.push_back(entry.clone());

        return clonedVector;
    }

private:
    std::size_t totalByteSize() const
    {
        std::size_t totalSize = 0;

        for (auto &&string : *this)
            totalSize += string.size();

        return totalSize;
    }
};

template <typename Type>
std::vector<Type> clone(const std::vector<Type> &vector)
{
    std::vector<Type> clonedVector;
    clonedVector.reserve(vector.size());

    for (auto &&entry : vector)
        clonedVector.push_back(entry.clone());

    return clonedVector;
}

inline QDataStream &operator<<(QDataStream &out, const SmallStringVector &stringVector)
{
    out << quint64(stringVector.size());

    for (auto &&string : stringVector)
        out << string;

    return out;
}

inline QDataStream &operator>>(QDataStream &in, SmallStringVector &stringVector)
{
    stringVector.clear();

    quint64 size;

    in >> size;

    stringVector.reserve(size);

    for (quint64 i = 0; i < size; ++i) {
        SmallString string;

        in >> string;

        stringVector.push_back(std::move(string));
    }

    return in;
}

inline QDebug operator<<(QDebug debug, const SmallStringVector &stringVector)
{
    debug << "StringVector(" << stringVector.join(Utils::SmallString(", ")).constData() << ")";

    return debug;
}

inline void PrintTo(const SmallStringVector &textVector, ::std::ostream* os)
{
    *os << "StringVector(" << textVector.join(Utils::SmallString(", ")).constData() << ")";
}

} // namespace Utils;

namespace std {

template<typename Type>
QDataStream &operator<<(QDataStream &out, const vector<Type> &vector)
{
    out << quint64(vector.size());

    for (auto &&entry : vector)
        out << entry;

    return out;
}

template<typename Type>
QDataStream &operator>>(QDataStream &in, vector<Type> &vector)
{
    vector.clear();

    quint64 size;

    in >> size;

    vector.reserve(size);

    for (quint64 i = 0; i < size; ++i) {
        Type entry;

        in >> entry;

        vector.push_back(move(entry));
    }

    return in;
}

} // namespace std

QT_BEGIN_NAMESPACE

template<typename Type>
QDebug &operator<<(QDebug &debug, const std::vector<Type> &vector)
{
    debug.noquote() << "[";
    for (auto &&entry : vector)
        debug.noquote() << entry << ", ";
    debug.noquote() << "]";

    return debug;
}

QT_END_NAMESPACE

#pragma pop_macro("noexcept")
