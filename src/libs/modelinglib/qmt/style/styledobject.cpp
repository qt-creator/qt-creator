// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "styledobject.h"

namespace qmt {

StyledObject::StyledObject(const DObject *object, const ObjectVisuals &objectVisuals,
                           const QList<const DObject *> &collidingObjects)
    : m_object(object),
      m_objectVisuals(objectVisuals),
      m_collidingObjects(collidingObjects)
{
}

StyledObject::~StyledObject()
{
}

} // namespace qmt
