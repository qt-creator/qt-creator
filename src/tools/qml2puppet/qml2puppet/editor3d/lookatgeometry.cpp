// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifdef QUICK3D_MODULE

#include "lookatgeometry.h"

namespace QmlDesigner::Internal {

LookAtGeometry::LookAtGeometry()
    : GeometryBase()
    , m_crossScale(1.f, 1.f, 1.f)
{
}

LookAtGeometry::~LookAtGeometry()
{
}

QVector3D LookAtGeometry::crossScale() const
{
    return m_crossScale;
}

void LookAtGeometry::setCrossScale(const QVector3D &scale)
{
    if (scale != m_crossScale) {
        m_crossScale = scale;
        emit crossScaleChanged();
        updateGeometry();
    }
}

void LookAtGeometry::doUpdateGeometry()
{
    GeometryBase::doUpdateGeometry();

    QByteArray vertexData;
    vertexData.resize(6 * 3 * 4); // 6 vertices of 3 floats each 4 bytes
    float *dataPtr = reinterpret_cast<float *>(vertexData.data());

    auto addVertex = [&dataPtr](float x, float y, float z) {
        dataPtr[0] = x;
        dataPtr[1] = y;
        dataPtr[2] = z;
        dataPtr += 3;
    };

    addVertex(m_crossScale.x(), 0.f, 0.f);
    addVertex(-m_crossScale.x(), 0.f, 0.f);
    addVertex(0.f, m_crossScale.y(), 0.f);
    addVertex(0.f, -m_crossScale.y(), 0.f);
    addVertex(0.f, 0.f, m_crossScale.z());
    addVertex(0.f, 0.f, -m_crossScale.z());

    setVertexData(vertexData);
    setBounds(-m_crossScale, m_crossScale);
}

} // namespace QmlDesigner::Internal

#endif // QUICK3D_MODULE
