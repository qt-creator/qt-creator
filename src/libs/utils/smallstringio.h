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

#include <ostream>

namespace Utils {

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
    using QT_PREPEND_NAMESPACE(operator<<);

    debug.nospace() << "\"" << string.data() << "\"";

    return debug;
}

inline
std::ostream &operator<<(std::ostream &stream, const SmallString &string)
{
    using std::operator<<;

    return stream << std::string(string.data());
}

inline
void PrintTo(const SmallString &string, ::std::ostream *os)
{
    *os << "'" << string.data() << "'";
}

inline QDataStream &operator<<(QDataStream &out, const SmallStringVector &stringVector)
{
    out << quint64(stringVector.size());

    for (auto &&string : stringVector)
        out << string;

    return out;
}

inline
QDataStream &operator>>(QDataStream &in, SmallStringVector &stringVector)
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

inline
QDebug operator<<(QDebug debug, const SmallStringVector &stringVector)
{
    debug << "StringVector(" << stringVector.join(Utils::SmallString(", ")).constData() << ")";

    return debug;
}

inline
void PrintTo(const SmallStringVector &textVector, ::std::ostream* os)
{
    *os << "StringVector(" << textVector.join(Utils::SmallString(", ")).constData() << ")";
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

} // namespace std
