// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#ifdef QUICK3D_MODULE

// This is a dummy class for Qt 5 compatibility purposes only

#include <QtQuick3D/private/qquick3dabstractlight_p.h>

namespace QmlDesigner::Internal {

class QQuick3DAreaLight : public QQuick3DAbstractLight
{
    Q_OBJECT
    Q_PROPERTY(float width READ width WRITE setWidth NOTIFY widthChanged)
    Q_PROPERTY(float height READ height WRITE setHeight NOTIFY heightChanged)

public:
    ~QQuick3DAreaLight() override {}

    float width() const;
    float height() const;

public slots:
    void setWidth(float width);
    void setHeight(float height);

signals:
    void widthChanged();
    void heightChanged();

private:
    float m_width = 100.0f;
    float m_height = 100.0f;
};

}

QML_DECLARE_TYPE(QmlDesigner::Internal::QQuick3DAreaLight)

#endif
