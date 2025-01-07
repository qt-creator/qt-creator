// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#ifdef QUICK3D_MODULE

#include "geometrybase.h"

#include <QtGui/QVector3D>

namespace QmlDesigner::Internal {

class LookAtGeometry : public GeometryBase
{
    Q_OBJECT
    Q_PROPERTY(QVector3D crossScale READ crossScale WRITE setCrossScale NOTIFY crossScaleChanged)

public:
    LookAtGeometry();
    ~LookAtGeometry() override;

    QVector3D crossScale() const;

public slots:
    void setCrossScale(const QVector3D &pos);

signals:
    void crossScaleChanged();

protected:
    void doUpdateGeometry() override;

private:
    QVector3D m_crossScale;
};

} // namespace QmlDesigner::Internal

QML_DECLARE_TYPE(QmlDesigner::Internal::LookAtGeometry)

#endif // QUICK3D_MODULE
