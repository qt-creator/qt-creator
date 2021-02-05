/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#include "linegeometry.h"

#include <QtQuick3DRuntimeRender/private/qssgrendergeometry_p.h>

namespace QmlDesigner {
namespace Internal {

LineGeometry::LineGeometry()
    : QQuick3DGeometry()
{
}

LineGeometry::~LineGeometry()
{
}

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
QString LineGeometry::name() const
{
    return objectName();
}

void LineGeometry::setName(const QString &name)
{
    setObjectName(name);
    emit nameChanged();
}
#endif

QVector3D LineGeometry::startPos() const
{
    return m_startPos;
}

QVector3D LineGeometry::endPos() const
{
    return m_endPos;
}

void LineGeometry::setStartPos(const QVector3D &pos)
{
    if (pos != m_startPos) {
        m_startPos = pos;
        emit startPosChanged();
        update();
    }
}

void LineGeometry::setEndPos(const QVector3D &pos)
{
    if (pos != m_endPos) {
        m_endPos = pos;
        emit endPosChanged();
        update();
    }
}

QSSGRenderGraphObject *LineGeometry::updateSpatialNode(QSSGRenderGraphObject *node)
{
    node = QQuick3DGeometry::updateSpatialNode(node);
    QSSGRenderGeometry *geometry = static_cast<QSSGRenderGeometry *>(node);
    geometry->clear();

    QByteArray vertexData;
    vertexData.resize(2 * 3 * 4); // 2 vertices of 3 floats each 4 bytes
    float *dataPtr = reinterpret_cast<float *>(vertexData.data());

    dataPtr[0] = m_startPos[0];
    dataPtr[1] = m_startPos[1];
    dataPtr[2] = m_startPos[2];
    dataPtr[3] = m_endPos[0];
    dataPtr[4] = m_endPos[1];
    dataPtr[5] = m_endPos[2];

    geometry->setStride(12);
#if QT_VERSION < QT_VERSION_CHECK(6, 1, 0)
    geometry->addAttribute(QSSGRenderGeometry::Attribute::PositionSemantic, 0,
                           QSSGRenderGeometry::Attribute::ComponentType::F32Type);
    geometry->setPrimitiveType(QSSGRenderGeometry::Lines);
#else
    geometry->addAttribute(QSSGMesh::RuntimeMeshData::Attribute::PositionSemantic, 0,
                           QSSGMesh::Mesh::ComponentType::Float32);
    geometry->setPrimitiveType(QSSGMesh::Mesh::DrawMode::Lines);
#endif
    geometry->setVertexData(vertexData);
    geometry->setBounds(m_startPos, m_endPos);

    return node;
}

}
}

#endif // QUICK3D_MODULE
