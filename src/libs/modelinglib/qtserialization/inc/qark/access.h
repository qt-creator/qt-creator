/****************************************************************************
**
** Copyright (C) 2016 Jochen Becher
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

namespace qark {

template<class Archive, class T>
class Access
{
public:
    static void save(Archive &archive, const T &t)
    {
        serializeHelper(archive, const_cast<T &>(t));
    }

    static void load(Archive &archive, T &t)
    {
        serializeHelper(archive, t);
    }

    static void serialize(Archive &archive, T &t);
};

template<class Archive, class T>
void save(Archive &archive, const T &t)
{
    Access<Archive, T>::save(archive, t);
}

template<class Archive, class T>
void save(Archive &archive, const T &t, const Parameters &)
{
    save(archive, t);
}

template<class Archive, class T>
void load(Archive &archive, T &t)
{
    Access<Archive, T>::load(archive, t);
}

template<class Archive, class T>
void load(Archive &archive, T &t, const Parameters &)
{
    load(archive, t);
}

template<class Archive, class T>
void serialize(Archive &archive, T &t)
{
    Access<Archive, T>::serialize(archive, t);
}

template<class Archive, class T>
void serializeHelper(Archive &archive, T &t)
{
    serialize(archive, t);
}

} // namespace qark

#define QARK_ACCESS(TYPE) \
    template<class Archive> \
    class Access<Archive, TYPE> { \
    public: \
        static inline void save(Archive &archive, const TYPE &); \
        static inline void load(Archive &archive, TYPE &); \
    };

#define QARK_ACCESS_SERIALIZE(TYPE) \
    template<class Archive> \
    class Access<Archive, TYPE> { \
    public: \
        static inline void save(Archive &archive, const TYPE &t) \
        { \
            serializeHelper(archive, const_cast<TYPE &>(t)); \
        } \
        static inline void load(Archive &archive, TYPE &t) { serializeHelper(archive, t); } \
        static inline void serialize(Archive &archive, TYPE &); \
    };

#define QARK_ACCESS_SPECIALIZE(INARCHIVE, OUTARCHIVE, TYPE) \
    template class Access<INARCHIVE, TYPE>; \
    template class Access<OUTARCHIVE, TYPE>;
