/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#include <QtQuick3D/private/qquick3dgeometry_p.h>

#include <QTimer>

namespace QmlDesigner {
namespace Internal {

class GeometryBase : public QQuick3DGeometry
{
    Q_OBJECT

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    // Name property was removed in Qt 6, so define it here for compatibility.
    // Name maps to object name.
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
public:
    QString name() const;
    void setName(const QString &name);
signals:
    void nameChanged();
#endif

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
