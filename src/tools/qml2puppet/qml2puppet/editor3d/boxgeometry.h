// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#ifdef QUICK3D_MODULE

#include "geometrybase.h"

#include <QVector3D>

namespace QmlDesigner::Internal {

class BoxGeometry : public GeometryBase
{
    Q_OBJECT
    Q_PROPERTY(QVector3D extent READ extent WRITE setExtent NOTIFY extentChanged)

public:

    BoxGeometry();
    ~BoxGeometry() override;

    QVector3D extent() const;
    void setExtent(const QVector3D &extent);

signals:
    void extentChanged();

protected:
    void doUpdateGeometry() override;

private:
    void fillVertexData(QByteArray &vertexData, QByteArray &indexData, const QVector3D &bounds);
    QVector3D m_extent = { 1.f, 1.f, 1.f };
};

} // namespace QmlDesigner::Internal

QML_DECLARE_TYPE(QmlDesigner::Internal::BoxGeometry)

#endif // QUICK3D_MODULE
