// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#ifdef QUICK3D_MODULE

#include <QtQuick3D/private/qquick3dgeometry_p.h>

#include <QTimer>

namespace QmlDesigner {
namespace Internal {

class GeometryBase : public QQuick3DGeometry
{
    Q_OBJECT

    // Name property was removed in Qt 6, so define it here for compatibility.
    // Name maps to object name.
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
public:
    QString name() const;
    void setName(const QString &name);
signals:
    void nameChanged();

public:
    GeometryBase();
    ~GeometryBase() override;

protected:
    void updateGeometry();
    virtual void doUpdateGeometry();

private:
    QTimer m_updatetimer;
};

}
}

QML_DECLARE_TYPE(QmlDesigner::Internal::GeometryBase)

#endif // QUICK3D_MODULE
