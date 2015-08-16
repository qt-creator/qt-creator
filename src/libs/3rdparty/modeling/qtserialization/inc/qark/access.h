/***************************************************************************
**
** Copyright (C) 2015 Jochen Becher
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef QARK_ACCESS_H
#define QARK_ACCESS_H

namespace qark {

template<class Archive, class T>
class Access {
public:
    static void save(Archive &archive, const T &t)
    {
        serialize_helper(archive, const_cast<T &>(t));
    }

    static void load(Archive &archive, T &t)
    {
        serialize_helper(archive, t);
    }

    static void serialize(Archive &archive, T &t);
};

template<class Archive, class T>
void save(Archive &archive, const T &t)
{
    Access<Archive, T>::save(archive, t);
}

template<class Archive, class T>
void load(Archive &archive, T &t)
{
    Access<Archive, T>::load(archive, t);
}

template<class Archive, class T>
void serialize(Archive &archive, T &t)
{
    Access<Archive, T>::serialize(archive, t);
}

template<class Archive, class T>
void serialize_helper(Archive &archive, T &t)
{
    serialize(archive, t);
}

}

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
        static inline void save(Archive &archive, const TYPE &t) { serialize_helper(archive, const_cast<TYPE &>(t)); } \
        static inline void load(Archive &archive, TYPE &t) { serialize_helper(archive, t); } \
        static inline void serialize(Archive &archive, TYPE &); \
    };

#if 0
#define QARK_ACCESS_SPECIALIZE_LOAD_SAVE(INARCHIVE, OUTARCHIVE, TYPE) \
    template<> class Access<INARCHIVE, TYPE> { public: static inline void load(INARCHIVE &archive, TYPE &); void serialize(INARCHIVE &, TYPE &); }; \
    template<> class Access<OUTARCHIVE, TYPE> { public: static inline void save(OUTARCHIVE &archive, const TYPE &); void serialize(OUTARCHIVE &, TYPE &); }; \
    void Access<INARCHIVE, TYPE>::serialize(INARCHIVE &, TYPE &) { } \
    void Access<OUTARCHIVE, TYPE>::serialize(OUTARCHIVE &, TYPE &) { } \
    template class Access<INARCHIVE, TYPE>; \
    template class Access<OUTARCHIVE, TYPE>;
#endif

#define QARK_ACCESS_SPECIALIZE(INARCHIVE, OUTARCHIVE, TYPE) \
    template class Access<INARCHIVE, TYPE>; \
    template class Access<OUTARCHIVE, TYPE>;

#if 0
#define QARK_SPECIALIZE_SERIALIZE(INARCHIVE, OUTARCHIVE, TYPE) \
    QARK_ACCESS_SPECIALIZE(INARCHIVE, OUTARCHIVE, TYPE); \
    template void serialize<INARCHIVE, TYPE>(INARCHIVE &, TYPE &); \
    template void serialize<OUTARCHIVE, TYPE>(OUTARCHIVE &, TYPE &);

#define QARK_SPECIALIZE_LOAD_SAVE(INARCHIVE, OUTARCHIVE, TYPE) \
    template void load<INARCHIVE, TYPE>(INARCHIVE &, TYPE &); \
    template void save<OUTARCHIVE, TYPE>(OUTARCHIVE &, const TYPE &);
#endif


#endif // QARK_ACCESS_H
