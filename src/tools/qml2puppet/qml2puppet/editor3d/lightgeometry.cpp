// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifdef QUICK3D_MODULE

#include "lightgeometry.h"

#include <QtQuick3DRuntimeRender/private/qssgrendergeometry_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderlight_p.h>
#include <QtCore/qmath.h>

#include <limits>

namespace QmlDesigner {
namespace Internal {

LightGeometry::LightGeometry()
    : GeometryBase()
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
    updateGeometry();
}

void LightGeometry::doUpdateGeometry()
{
    if (m_lightType == LightType::Invalid)
        return;

    GeometryBase::doUpdateGeometry();

    QByteArray vertexData;
    QByteArray indexData;
    QVector3D minBounds;
    QVector3D maxBounds;

    fillVertexData(vertexData, indexData, minBounds, maxBounds);

    addAttribute(QQuick3DGeometry::Attribute::IndexSemantic, 0,
                 QQuick3DGeometry::Attribute::U16Type);

    setVertexData(vertexData);
    setIndexData(indexData);
    setBounds(minBounds, maxBounds);
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
