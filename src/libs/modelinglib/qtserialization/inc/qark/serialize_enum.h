// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "parameters.h"

#include <type_traits>

namespace qark {

#if 0 // TODO this is ambigous with default implementation in access.h

template<class Archive, typename T>
inline typename std::enable_if<std::is_enum<T>::value, void>::type save(
        Archive &archive, const T &value, const Parameters &)
{
    archive.write(static_cast<int>(value));
}

template<class Archive, typename T>
inline typename std::enable_if<std::is_enum<T>::value, void>::type load(
        Archive &archive, T &value, const Parameters &)
{
    int i = 0;
    archive.read(&i);
    value = (T) i;
}

#endif

#define QARK_SERIALIZE_ENUM(ENUM) \
    template<class Archive> \
    inline void save(Archive &archive, const ENUM &e, const Parameters &) \
    { \
        archive.write(static_cast<int>(e)); \
    } \
    template<class Archive> \
    inline void load(Archive &archive, ENUM &e, const Parameters &) \
    { \
        int i = 0; \
        archive.read(&i); \
        e = (ENUM) i; \
    }

template<class Archive, typename T>
inline void save(Archive &archive, const QFlags<T> &flags, const Parameters &)
{
    archive.write((typename QFlags<T>::Int) flags);
}

template<class Archive, typename T>
inline void load(Archive &archive, QFlags<T> &flags, const Parameters &)
{
    typename QFlags<T>::Int i = 0;
    archive.read(&i);
    flags = QFlags<T>(i);
}

} // namespace qark
