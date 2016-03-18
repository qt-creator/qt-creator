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
