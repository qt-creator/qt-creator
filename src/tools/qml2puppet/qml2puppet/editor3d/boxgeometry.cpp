// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifdef QUICK3D_MODULE

#include "boxgeometry.h"

namespace QmlDesigner::Internal {

BoxGeometry::BoxGeometry()
    : GeometryBase()
{
}

BoxGeometry::~BoxGeometry()
{
}

QVector3D BoxGeometry::extent() const
{
    return m_extent;
}

void BoxGeometry::setExtent(const QVector3D &extent)
{
    if (m_extent == extent)
        return;

    m_extent = extent;

    emit extentChanged();
    updateGeometry();
}

void BoxGeometry::doUpdateGeometry()
{
    GeometryBase::doUpdateGeometry();

    QByteArray vertexData;
    QByteArray indexData;
    const QVector3D bounds = m_extent / 2.f;

    fillVertexData(vertexData, indexData, bounds);

    addAttribute(QQuick3DGeometry::Attribute::IndexSemantic, 0,
                 QQuick3DGeometry::Attribute::U16Type);

    setVertexData(vertexData);
    setIndexData(indexData);
    setBounds(-bounds, bounds);
}

void BoxGeometry::fillVertexData(QByteArray &vertexData, QByteArray &indexData,
                                 const QVector3D &bounds)
{
    int vertexSize = 8 * 3 * int(sizeof(float));
    int indexSize = 12 * 2 * int(sizeof(quint16));

    vertexData.resize(vertexSize);
    indexData.resize(indexSize);

    auto dataPtr = reinterpret_cast<float *>(vertexData.data());
    auto indexPtr = reinterpret_cast<quint16 *>(indexData.data());

    QVector3D corners[8];

    corners[0] = QVector3D( bounds.x(),  bounds.y(),  bounds.z());
    corners[1] = QVector3D(-bounds.x(),  bounds.y(),  bounds.z());
    corners[2] = QVector3D(-bounds.x(), -bounds.y(),  bounds.z());
    corners[3] = QVector3D( bounds.x(), -bounds.y(),  bounds.z());
    corners[4] = QVector3D( bounds.x(),  bounds.y(), -bounds.z());
    corners[5] = QVector3D(-bounds.x(),  bounds.y(), -bounds.z());
    corners[6] = QVector3D(-bounds.x(), -bounds.y(), -bounds.z());
    corners[7] = QVector3D( bounds.x(), -bounds.y(), -bounds.z());

    for (int i = 0; i < 8; ++i) {
        const QVector3D &vert = corners[i];
        *dataPtr++ = vert.x(); *dataPtr++ = vert.y(); *dataPtr++ = vert.z();
    }

    auto addLine = [&](int start, int end) {
        *indexPtr++ = start; *indexPtr++ = end;
    };

    addLine(0, 1);
    addLine(1, 2);
    addLine(2, 3);
    addLine(3, 0);

    addLine(4, 5);
    addLine(5, 6);
    addLine(6, 7);
    addLine(7, 4);

    addLine(0, 4);
    addLine(1, 5);
    addLine(2, 6);
    addLine(3, 7);
}

} // namespace QmlDesigner::Internal

#endif // QUICK3D_MODULE
