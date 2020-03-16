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
#include <QtQuick3D/private/qquick3darealight_p.h>
#include <QtQuick3D/private/qquick3ddirectionallight_p.h>
#include <QtQuick3D/private/qquick3dpointlight_p.h>
#include <QtQuick3D/private/qquick3dspotlight_p.h>
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

QQuick3DAbstractLight *QmlDesigner::Internal::LightGeometry::light() const
{
    return m_light;
}

void QmlDesigner::Internal::LightGeometry::setLight(QQuick3DAbstractLight *light)
{
    if (m_light == light)
        return;

    m_light = light;

    emit lightChanged();
    update();
}

QSSGRenderGraphObject *LightGeometry::updateSpatialNode(QSSGRenderGraphObject *node)
{
    if (!m_light)
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
    const int dirSegments = 12; // Segment lines in directional light circle
    const int spotArc = 6; // Segment lines per cone line in spotlight arc
    const int spotCone = 4; // Lines in spotlight cone
    const int pointLightDensity = 5;

    if (qobject_cast<QQuick3DAreaLight *>(m_light)) {
        // Area light model is a rectangle with perpendicular lines on corners
        vertexSize = int(sizeof(float)) * 3 * 8;
        indexSize = int(sizeof(quint16)) * 8 * 2;
    } else if (qobject_cast<QQuick3DDirectionalLight *>(m_light)) {
        // Directional light model is a circle with perpendicular lines on circumference vertices
        vertexSize = int(sizeof(float)) * 3 * dirSegments * 2;
        indexSize = int(sizeof(quint16)) * dirSegments * 2 * 2;
    } else if (qobject_cast<QQuick3DPointLight *>(m_light)) {
        // Point light model is a set of lines radiating from central point.
        // We reserve more than we need so we don't have to calculate the actual need here,
        // and resize later when we know the exact count.
        vertexSize = int(sizeof(float)) * 3 * pointLightDensity * pointLightDensity * 4;
        indexSize = int(sizeof(quint16)) * pointLightDensity * pointLightDensity * 4;
    } else if (qobject_cast<QQuick3DSpotLight *>(m_light)) {
        vertexSize = int(sizeof(float)) * 3 * (spotArc * spotCone + 1);
        indexSize = int(sizeof(quint16)) * (spotArc + 1) * spotCone * 2;
    }
    vertexData.resize(vertexSize);
    indexData.resize(indexSize);

    auto dataPtr = reinterpret_cast<float *>(vertexData.data());
    auto indexPtr = reinterpret_cast<quint16 *>(indexData.data());

    if (qobject_cast<QQuick3DAreaLight *>(m_light)) {
        *dataPtr++ = -1.f; *dataPtr++ = 1.f;  *dataPtr++ = 0.f;
        *dataPtr++ = -1.f; *dataPtr++ = -1.f; *dataPtr++ = 0.f;
        *dataPtr++ = 1.f;  *dataPtr++ = -1.f; *dataPtr++ = 0.f;
        *dataPtr++ = 1.f;  *dataPtr++ = 1.f;  *dataPtr++ = 0.f;

        *dataPtr++ = -1.f; *dataPtr++ = 1.f;  *dataPtr++ = -1.f;
        *dataPtr++ = -1.f; *dataPtr++ = -1.f; *dataPtr++ = -1.f;
        *dataPtr++ = 1.f;  *dataPtr++ = -1.f; *dataPtr++ = -1.f;
        *dataPtr++ = 1.f;  *dataPtr++ = 1.f;  *dataPtr++ = -1.f;

        *indexPtr++ = 0; *indexPtr++ = 1;
        *indexPtr++ = 1; *indexPtr++ = 2;
        *indexPtr++ = 2; *indexPtr++ = 3;
        *indexPtr++ = 3; *indexPtr++ = 0;

        *indexPtr++ = 0; *indexPtr++ = 4;
        *indexPtr++ = 1; *indexPtr++ = 5;
        *indexPtr++ = 2; *indexPtr++ = 6;
        *indexPtr++ = 3; *indexPtr++ = 7;
    } else if (qobject_cast<QQuick3DDirectionalLight *>(m_light)) {
        const double segment = M_PI * 2. / double(dirSegments);
        for (quint16 i = 0; i < dirSegments; ++i) {
            float x = float(qCos(i * segment));
            float y = float(qSin(i * segment));
            *dataPtr++ = x; *dataPtr++ = y; *dataPtr++ = 0.f;
            *dataPtr++ = x; *dataPtr++ = y; *dataPtr++ = -1.f;
            const quint16 base = i * 2;
            *indexPtr++ = base; *indexPtr++ = base + 1;
            *indexPtr++ = base; *indexPtr++ = base + 2;
        }
        // Adjust the final index to complete the circle
        *(--indexPtr) = 0;
    } else if (qobject_cast<QQuick3DPointLight *>(m_light)) {
        const double innerRad = .3;
        vertexSize = 0;
        indexSize = 0;
        int vertexIndex = 0;
        for (quint16 i = 0; i < pointLightDensity; ++i) {
            double latAngle = (((.9 / (pointLightDensity - 1)) * i) + .05) * M_PI;
            quint16 longPoints = pointLightDensity * 2 * qSin(latAngle);
            latAngle -= M_PI_2;
            const double longSegment = M_PI * 2. / double(longPoints);
            for (quint16 j = 0; j < longPoints; ++j) {
                double longAngle = longSegment * j;
                float q = float(qCos(latAngle));
                float x = float(qCos(longAngle) * q);
                float y = float(qSin(latAngle));
                float z = float(qSin(longAngle) * q);

                *dataPtr++ = x * innerRad; *dataPtr++ = y * innerRad; *dataPtr++ = z * innerRad;
                *dataPtr++ = x; *dataPtr++ = y; *dataPtr++ = z;
                *indexPtr++ = vertexIndex; *indexPtr++ = vertexIndex + 1;

                vertexIndex += 2;
                vertexSize += 6 * sizeof(float);
                indexSize += 2 * sizeof(quint16);
            }
        }
        vertexData.resize(vertexSize);
        indexData.resize(indexSize);
    } else if (qobject_cast<QQuick3DSpotLight *>(m_light)) {
        const quint16 segments = spotArc * spotCone;
        const double segment = M_PI * 2. / double(segments);

        // Circle
        for (quint16 i = 0; i < segments; ++i) {
            float x = float(qCos(i * segment));
            float y = float(qSin(i * segment));
            *dataPtr++ = x; *dataPtr++ = y; *dataPtr++ = -2.f;
            *indexPtr++ = i; *indexPtr++ = i + 1;
        }
        // Adjust the final index to complete the circle
        *(indexPtr - 1) = 0;

        // Cone tip
        *dataPtr++ = 0.f; *dataPtr++ = 0.f; *dataPtr++ = 0.f;
        quint16 tipIndex = segments;

        // Cone lines
        for (quint16 i = 0; i < spotCone; ++i) {
            *indexPtr++ = tipIndex;
            *indexPtr++ = i * spotArc;
        }
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
