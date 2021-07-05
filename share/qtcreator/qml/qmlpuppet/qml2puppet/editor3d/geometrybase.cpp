/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#ifdef QUICK3D_MODULE

#include "geometrybase.h"

namespace QmlDesigner {
namespace Internal {

GeometryBase::GeometryBase()
    : QQuick3DGeometry()
{
    m_updatetimer.setSingleShot(true);
    m_updatetimer.setInterval(0);
    connect(&m_updatetimer, &QTimer::timeout, this, &GeometryBase::doUpdateGeometry);
    updateGeometry();
    setStride(12); // To avoid div by zero inside QtQuick3D
}

GeometryBase::~GeometryBase()
{
}

void GeometryBase::doUpdateGeometry()
{
    clear();

    setStride(12);

    addAttribute(QQuick3DGeometry::Attribute::PositionSemantic, 0,
                 QQuick3DGeometry::Attribute::F32Type);
    setPrimitiveType(QQuick3DGeometry::PrimitiveType::Lines);

    update();
}

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
QString GeometryBase::name() const
{
    return objectName();
}

void GeometryBase::setName(const QString &name)
{
    setObjectName(name);
    emit nameChanged();
}
#endif

void GeometryBase::updateGeometry()
{
    m_updatetimer.start();
}

}
}

#endif // QUICK3D_MODULE
