// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
