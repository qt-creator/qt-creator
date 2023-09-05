// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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

QString GeometryBase::name() const
{
    return objectName();
}

void GeometryBase::setName(const QString &name)
{
    setObjectName(name);
    emit nameChanged();
}

void GeometryBase::updateGeometry()
{
    m_updatetimer.start();
}

}
}

#endif // QUICK3D_MODULE
