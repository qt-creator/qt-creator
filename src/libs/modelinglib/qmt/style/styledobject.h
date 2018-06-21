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
