// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifdef QUICK3D_MODULE

#include "linegeometry.h"

namespace QmlDesigner {
namespace Internal {

LineGeometry::LineGeometry()
    : GeometryBase()
{
}

LineGeometry::~LineGeometry()
{
}

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
        updateGeometry();
    }
}

void LineGeometry::setEndPos(const QVector3D &pos)
{
    if (pos != m_endPos) {
        m_endPos = pos;
        emit endPosChanged();
        updateGeometry();
    }
}

void LineGeometry::doUpdateGeometry()
{
    GeometryBase::doUpdateGeometry();

    QByteArray vertexData;
    vertexData.resize(2 * 3 * 4); // 2 vertices of 3 floats each 4 bytes
    float *dataPtr = reinterpret_cast<float *>(vertexData.data());

    dataPtr[0] = m_startPos[0];
    dataPtr[1] = m_startPos[1];
    dataPtr[2] = m_startPos[2];
    dataPtr[3] = m_endPos[0];
    dataPtr[4] = m_endPos[1];
    dataPtr[5] = m_endPos[2];

    setVertexData(vertexData);
    setBounds(m_startPos, m_endPos);
}

}
}

#endif // QUICK3D_MODULE
