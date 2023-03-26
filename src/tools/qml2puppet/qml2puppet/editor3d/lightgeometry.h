// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#ifdef QUICK3D_MODULE

#include "geometrybase.h"

namespace QmlDesigner {
namespace Internal {

class LightGeometry : public GeometryBase
{
    Q_OBJECT
    Q_PROPERTY(LightType lightType READ lightType WRITE setLightType NOTIFY lightTypeChanged)

public:
    enum class LightType {
        Invalid,
        Spot,
        Area,
        Directional,
        Point
    };
    Q_ENUM(LightType)

    LightGeometry();
    ~LightGeometry() override;

    LightType lightType() const;

public slots:
    void setLightType(LightType lightType);

signals:
    void lightTypeChanged();

protected:
    void doUpdateGeometry() override;

private:
    void fillVertexData(QByteArray &vertexData, QByteArray &indexData,
                        QVector3D &minBounds, QVector3D &maxBounds);
    LightType m_lightType = LightType::Invalid;
};

}
}

QML_DECLARE_TYPE(QmlDesigner::Internal::LightGeometry)

#endif // QUICK3D_MODULE
