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

#include "qmt/infrastructure/handle.h"
#include "qmt/infrastructure/handles.h"
#include "qmt/infrastructure/uid.h"

#include "qark/qxmloutarchive.h"
#include "qark/qxmlinarchive.h"
#include "qark/serialize.h"
#include "qark/access.h"

namespace qark {

// Uid

template<class Archive>
inline void save(Archive &archive, const qmt::Uid &uid)
{
    archive.write(uid.toString());
}

template<class Archive>
inline void load(Archive &archive, qmt::Uid &uid)
{
    QString s;
    archive.read(&s);
    uid.fromString(s);
}

// Handle

template<class Archive, class T>
inline void serialize(Archive &archive, qmt::Handle<T> &handle)
{
    archive || tag("handle", handle)
            || attr("uid", handle, &qmt::Handle<T>::uid, &qmt::Handle<T>::setUid)
            || attr("target", handle, &qmt::Handle<T>::target, &qmt::Handle<T>::setTarget)
            || end;
}

// Handles

template<class Archive, class T>
inline void serialize(Archive &archive, qmt::Handles<T> &handles)
{
    archive || tag("handles", handles)
            || attr("handles", handles, &qmt::Handles<T>::get, &qmt::Handles<T>::set)
            || end;
}

} // namespace qark
