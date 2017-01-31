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

#include <QStringList>

namespace Utils {

template<uint SmallStringSize>
class BasicSmallStringVector : public std::vector<BasicSmallString<SmallStringSize>>
{
    using SmallString = BasicSmallString<SmallStringSize>;
    using Base = std::vector<SmallString>;
public:
    BasicSmallStringVector() = default;

    explicit BasicSmallStringVector(const Base &stringVector)
        : Base(stringVector.begin(), stringVector.end())
    {
    }

    BasicSmallStringVector(std::initializer_list<SmallString> list)
    {
        Base::reserve(list.size());

        for (auto &&entry : list)
            Base::push_back(entry.clone());
    }

    explicit BasicSmallStringVector(const QStringList &stringList)
    {
        std::vector<SmallString>::reserve(std::size_t(stringList.count()));

        for (const QString &string : stringList)
            Base::push_back(SmallString::fromQString(string));
    }

    explicit BasicSmallStringVector(const std::vector<std::string> &stringVector)
    {
        Base::reserve(std::size_t(stringVector.size()));

        for (const std::string &string : stringVector)
           Base::emplace_back(string);
    }

    BasicSmallStringVector(const BasicSmallStringVector &) = default;
    BasicSmallStringVector &operator=(const BasicSmallStringVector &) = default;

    BasicSmallStringVector(BasicSmallStringVector &&) noexcept = default;
    BasicSmallStringVector &operator=(BasicSmallStringVector &&)
        noexcept(std::is_nothrow_move_assignable<Base>::value) = default;

    SmallString join(SmallString &&separator) const
    {
        SmallString joinedString;

        joinedString.reserve(totalByteSize() + separator.size() * std::size_t(Base::size()));

        for (auto stringIterator = Base::begin(); stringIterator != Base::end(); ++stringIterator) {
            joinedString.append(*stringIterator);
            if (std::next(stringIterator) != Base::end())
                joinedString.append(separator);
        }

        return joinedString;
    }

    bool contains(SmallStringView string) const noexcept
    {
        return std::find(Base::begin(), Base::end(), string) != Base::end();
    }

    bool removeFast(SmallStringView valueToBeRemoved)
    {
        auto position = std::remove(Base::begin(), Base::end(), valueToBeRemoved);

        const bool hasEntry = position != Base::end();

        erase(position, Base::end());

        return hasEntry;
    }

    void append(SmallString &&string)
    {
        push_back(std::move(string));
    }

    BasicSmallStringVector clone() const
    {
        BasicSmallStringVector clonedVector;
        clonedVector.reserve(Base::size());

        for (auto &&entry : *this)
            clonedVector.push_back(entry.clone());

        return clonedVector;
    }

    operator std::vector<std::string>() const
    {
        return std::vector<std::string>(Base::begin(), Base::end());
    }

    operator QStringList() const
    {
        QStringList qStringList;
        qStringList.reserve(int(Base::size()));

        std::copy(Base::begin(), Base::end(), std::back_inserter(qStringList));

        return qStringList;
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

using SmallStringVector = BasicSmallStringVector<31>;
using PathStringVector = BasicSmallStringVector<190>;
} // namespace Utils;
