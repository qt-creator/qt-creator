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

#include "lightgeometry.h"

#include <QtQuick3DRuntimeRender/private/qssgrendergeometry_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderlight_p.h>
#include <QtCore/qmath.h>

#include <limits>

namespace QmlDesigner {
namespace Internal {

LightGeometry::LightGeometry()
    : QQuick3DGeometry()
{
}

LightGeometry::~LightGeometry()
{
}

LightGeometry::LightType LightGeometry::lightType() const
{
    return m_lightType;
}

void LightGeometry::setLightType(LightGeometry::LightType lightType)
{
    if (m_lightType == lightType)
        return;

    m_lightType = lightType;

    emit lightTypeChanged();
    update();
}

QSSGRenderGraphObject *LightGeometry::updateSpatialNode(QSSGRenderGraphObject *node)
{
    if (m_lightType == LightType::Invalid)
        return node;

    node = QQuick3DGeometry::updateSpatialNode(node);
    QSSGRenderGeometry *geometry = static_cast<QSSGRenderGeometry *>(node);

    geometry->clear();

    QByteArray vertexData;
    QByteArray indexData;
    QVector3D minBounds;
    QVector3D maxBounds;
    fillVertexData(vertexData, indexData, minBounds, maxBounds);

    geometry->addAttribute(QSSGRenderGeometry::Attribute::PositionSemantic, 0,
                           QSSGRenderGeometry::Attribute::ComponentType::F32Type);
    geometry->addAttribute(QSSGRenderGeometry::Attribute::IndexSemantic, 0,
                           QSSGRenderGeometry::Attribute::ComponentType::U16Type);
    geometry->setStride(12);
    geometry->setVertexData(vertexData);
    geometry->setIndexData(indexData);
    geometry->setPrimitiveType(QSSGRenderGeometry::Lines);
    geometry->setBounds(minBounds, maxBounds);

    return node;
}

void LightGeometry::fillVertexData(QByteArray &vertexData, QByteArray &indexData,
                                   QVector3D &minBounds, QVector3D &maxBounds)
{
    int vertexSize = 0;
    int indexSize = 0;
    const int arc = 12; // Segment lines per cone line in spot/directional light arc
    const int dirLines = 4; // Directional lines in spot/directional light
    const quint16 segments = arc * dirLines;
    const double segment = M_PI * 2. / double(segments);

    if (m_lightType == LightType::Area) {
        // Area light model is a rectangle
        vertexSize = int(sizeof(float)) * 3 * 4;
        indexSize = int(sizeof(quint16)) * 4 * 2;
    } else if (m_lightType == LightType::Directional) {
        // Directional light model is a circle with perpendicular lines on circumference vertices
        vertexSize = int(sizeof(float)) * 3 * (segments + dirLines);
        indexSize = int(sizeof(quint16)) * (segments + dirLines) * 2;
    } else if (m_lightType == LightType::Spot) {
        vertexSize = int(sizeof(float)) * 3 * (segments + 1);
        indexSize = int(sizeof(quint16)) * (segments + dirLines) * 2;
    } else if (m_lightType == LightType::Point) {
        vertexSize = int(sizeof(float)) * 3 * segments;
        indexSize = int(sizeof(quint16)) * segments * 2;
    }
    vertexData.resize(vertexSize);
    indexData.resize(indexSize);

    auto dataPtr = reinterpret_cast<float *>(vertexData.data());
    auto indexPtr = reinterpret_cast<quint16 *>(indexData.data());

    auto createCircle = [&](quint16 startIndex, float zVal, int xIdx, int yIdx, int zIdx) {
        for (quint16 i = 0; i < segments; ++i) {
            float x = float(qCos(i * segment));
            float y = float(qSin(i * segment));
            auto vecPtr = reinterpret_cast<QVector3D *>(dataPtr);
            (*vecPtr)[xIdx] = x;
            (*vecPtr)[yIdx] = y;
            (*vecPtr)[zIdx] = zVal;
            dataPtr += 3;
            *indexPtr++ = startIndex + i; *indexPtr++ = startIndex + i + 1;
        }
        // Adjust the final index to complete the circle
        *(indexPtr - 1) = startIndex;
    };

    if (m_lightType == LightType::Area) {
        *dataPtr++ = -1.f; *dataPtr++ = 1.f;  *dataPtr++ = 0.f;
        *dataPtr++ = -1.f; *dataPtr++ = -1.f; *dataPtr++ = 0.f;
        *dataPtr++ = 1.f;  *dataPtr++ = -1.f; *dataPtr++ = 0.f;
        *dataPtr++ = 1.f;  *dataPtr++ = 1.f;  *dataPtr++ = 0.f;

        *indexPtr++ = 0; *indexPtr++ = 1;
        *indexPtr++ = 1; *indexPtr++ = 2;
        *indexPtr++ = 2; *indexPtr++ = 3;
        *indexPtr++ = 3; *indexPtr++ = 0;
    } else if (m_lightType == LightType::Directional) {
        createCircle(0, 0.f, 0, 1, 2);

        // Dir lines
        for (quint16 i = 0; i < dirLines; ++i) {
            auto circlePtr = reinterpret_cast<float *>(vertexData.data()) + (3 * arc * i);
            *dataPtr++ = *circlePtr; *dataPtr++ = *(circlePtr + 1); *dataPtr++ = -3.f;
            *indexPtr++ = i * arc;
            *indexPtr++ = i + segments;
        }
    } else if (m_lightType == LightType::Spot) {
        createCircle(0, -1.f, 0, 1, 2);

        // Cone tip
        *dataPtr++ = 0.f; *dataPtr++ = 0.f; *dataPtr++ = 0.f;
        quint16 tipIndex = segments;

        // Cone lines
        for (quint16 i = 0; i < dirLines; ++i) {
            *indexPtr++ = tipIndex;
            *indexPtr++ = i * arc;
        }
    } else if (m_lightType == LightType::Point) {
        createCircle(0, 0.f, 0, 1, 2);
    }

    static const float floatMin = std::numeric_limits<float>::lowest();
    static const float floatMax = std::numeric_limits<float>::max();
    auto vertexPtr = reinterpret_cast<QVector3D *>(vertexData.data());
    minBounds = QVector3D(floatMax, floatMax, floatMax);
    maxBounds = QVector3D(floatMin, floatMin, floatMin);
    for (int i = 0; i < vertexSize / 12; ++i) {
        minBounds[0] = qMin((*vertexPtr)[0], minBounds[0]);
        minBounds[1] = qMin((*vertexPtr)[1], minBounds[1]);
        minBounds[2] = qMin((*vertexPtr)[2], minBounds[2]);
        maxBounds[0] = qMax((*vertexPtr)[0], maxBounds[0]);
        maxBounds[1] = qMax((*vertexPtr)[1], maxBounds[1]);
        maxBounds[2] = qMax((*vertexPtr)[2], maxBounds[2]);
        ++vertexPtr;
    }
}

}
}

#endif // QUICK3D_MODULE
