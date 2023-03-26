// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "objectvisuals.h"

#include <QList>

namespace qmt {

class DObject;

class QMT_EXPORT StyledObject
{
public:
    StyledObject(const DObject *object, const ObjectVisuals &objectVisuals,
                 const QList<const DObject *> &collidingObjects);
    ~StyledObject();

    const DObject *object() const { return m_object; }
    ObjectVisuals objectVisuals() const { return m_objectVisuals; }
    QList<const DObject *> collidingObjects() const { return m_collidingObjects; }

private:
    const DObject *m_object = nullptr;
    ObjectVisuals m_objectVisuals;
    QList<const DObject *> m_collidingObjects;
};

} // namespace qmt
