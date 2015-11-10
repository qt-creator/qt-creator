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

#ifndef QARK_SERIALIZE_ENUM_H
#define QARK_SERIALIZE_ENUM_H

#include "parameters.h"

#include <type_traits>

namespace qark {

#if 0 // ambigous with default implementation in access.h

template<class Archive, typename T>
inline typename std::enable_if<std::is_enum<T>::value, void>::type save(Archive &archive, const T &value, const Parameters &)
{
    archive.write(static_cast<int>(value));
}

template<class Archive, typename T>
inline typename std::enable_if<std::is_enum<T>::value, void>::type load(Archive &archive, T &value, const Parameters &)
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

}

#endif // QARK_SERIALIZE_ENUM_H
