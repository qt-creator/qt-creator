// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
