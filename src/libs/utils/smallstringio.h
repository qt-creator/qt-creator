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

#include "smallstringvector.h"

#include <QDataStream>
#include <QDebug>

#include <iterator>
#include <ostream>

#ifdef UNIT_TESTS
#include <gtest/gtest.h>
#endif

namespace Utils {

template <uint Size>
QDataStream &operator<<(QDataStream &out, const BasicSmallString<Size> &string)
{
   if (string.isEmpty())
       out << quint32(0);
   else
       out.writeBytes(string.data(), qint32(string.size()));

   return out;
}

template <uint Size>
QDataStream &operator>>(QDataStream &in, BasicSmallString<Size> &string)
{
    quint32 size;

    in >> size;

    if (size > 0 ) {
        string.resize(size);

        char *data = string.data();

        in.readRawData(data, int(size));
    }

    return in;
}

template <typename String>
QDebug &operator<<(QDebug &debug, const String &string)
{
    using QT_PREPEND_NAMESPACE(operator<<);

    debug.nospace().quote() << QByteArray::fromRawData(string.data(), int(string.size()));

    return debug;
}

template <uint Size>
std::ostream &operator<<(std::ostream &out, const BasicSmallString<Size> &string)
{
    BasicSmallString<Size> formatedString = string.clone();

    formatedString.replace("\n", "\\n");
    formatedString.replace("\t", "\\t");

    out << "\"";

    out.write(formatedString.data(), std::streamsize(formatedString.size()));

    out << "\"";

    return out;
}

inline
std::ostream &operator<<(std::ostream &out, SmallStringView string)
{
    return out << PathString(string);
}

template <typename String>
QDataStream &operator<<(QDataStream &out, const BasicSmallStringVector<String>  &stringVector)
{
    out << quint64(stringVector.size());

    for (auto &&string : stringVector)
        out << string;

    return out;
}

template <typename String>
QDataStream &operator>>(QDataStream &in, BasicSmallStringVector<String>  &stringVector)
{
    stringVector.clear();

    quint64 size;

    in >> size;

    stringVector.reserve(size);

    for (quint64 i = 0; i < size; ++i) {
        String string;

        in >> string;

        stringVector.push_back(std::move(string));
    }

    return in;
}

template <typename String>
QDebug operator<<(QDebug debug, const BasicSmallStringVector<String> &stringVector)
{
    debug << "StringVector(" << stringVector.join(", ").constData() << ")";

    return debug;
}

template <typename String>
std::ostream &operator<<(std::ostream &out, const BasicSmallStringVector<String> &textVector)
{
    return out << "[" << textVector.join("\", \"") << "]";
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

template<typename Key,
         typename Value,
         typename Hash = hash<Key>,
         typename KeyEqual = equal_to<Key>,
         typename Allocator = allocator<pair<const Key, Value>>>
QDataStream &operator<<(QDataStream &out, const unordered_map<Key, Value, Hash, KeyEqual, Allocator> &map)
{
    out << quint64(map.size());

    for (auto &&entry : map)
        out << entry.first << entry.second;

    return out;
}

template<typename Key,
         typename Value,
         typename Hash = hash<Key>,
         typename KeyEqual = equal_to<Key>,
         typename Allocator = allocator<pair<const Key, Value>>>
QDataStream &operator>>(QDataStream &in, unordered_map<Key, Value, Hash, KeyEqual, Allocator> &map)
{
    quint64 size;

    in >> size;

    map.reserve(size);

    for (quint64 i = 0; i < size; ++i) {
        Key key;
        Value value;

        in >> key >> value;

        map.insert(make_pair(move(key), move(value)));
    }

    return in;
}

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

#ifdef UNIT_TESTS
template <typename T>
ostream &operator<<(ostream &out, const vector<T> &vector)
{
    out << "[";

    ostream_iterator<string> outIterator(out, ", ");

    for (const auto &entry : vector)
        outIterator = ::testing::PrintToString(entry);

    out << "]";

    return out;
}
#else
template <typename T>
ostream &operator<<(ostream &out, const vector<T> &vector)
{
    out << "[";

    copy(vector.cbegin(), vector.cend(), ostream_iterator<T>(out, ", "));

    out << "]";

    return out;
}
#endif

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
