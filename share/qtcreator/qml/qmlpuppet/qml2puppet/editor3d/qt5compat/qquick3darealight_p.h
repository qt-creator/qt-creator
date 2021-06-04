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
