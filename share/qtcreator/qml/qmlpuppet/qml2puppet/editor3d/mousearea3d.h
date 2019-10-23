/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include <QtGui/qvector3d.h>
#include <QtCore/qpointer.h>

#include <QtQuick3D/private/qquick3dnode_p.h>
#include <QtQuick3D/private/qquick3dviewport_p.h>
#include <QtQuick3D/private/qtquick3dglobal_p.h>

namespace QmlDesigner {
namespace Internal {

class MouseArea3D : public QQuick3DNode
{
    Q_OBJECT
    Q_PROPERTY(QQuick3DViewport *view3D READ view3D WRITE setView3D NOTIFY view3DChanged)
    Q_PROPERTY(bool grabsMouse READ grabsMouse WRITE setGrabsMouse NOTIFY grabsMouseChanged)
    Q_PROPERTY(qreal x READ x WRITE setX NOTIFY xChanged)
    Q_PROPERTY(qreal y READ y WRITE setY NOTIFY yChanged)
    Q_PROPERTY(qreal width READ width WRITE setWidth NOTIFY widthChanged)
    Q_PROPERTY(qreal height READ height WRITE setHeight NOTIFY heightChanged)
    Q_PROPERTY(bool hovering READ hovering NOTIFY hoveringChanged)
    Q_PROPERTY(bool dragging READ dragging NOTIFY draggingChanged)

    Q_INTERFACES(QQmlParserStatus)

public:
    MouseArea3D(QQuick3DNode *parent = nullptr);

    QQuick3DViewport *view3D() const;

    qreal x() const;
    qreal y() const;
    qreal width() const;
    qreal height() const;

    bool hovering() const;
    bool dragging() const;
    bool grabsMouse() const;

public slots:
    void setView3D(QQuick3DViewport *view3D);
    void setGrabsMouse(bool grabsMouse);

    void setX(qreal x);
    void setY(qreal y);
    void setWidth(qreal width);
    void setHeight(qreal height);

    Q_INVOKABLE QVector3D rayIntersectsPlane(const QVector3D &rayPos0,
                                             const QVector3D &rayPos1,
                                             const QVector3D &planePos,
                                             const QVector3D &planeNormal) const;

signals:
    void view3DChanged();

    void xChanged(qreal x);
    void yChanged(qreal y);
    void widthChanged(qreal width);
    void heightChanged(qreal height);

    void hoveringChanged();
    void draggingChanged();
    void pressed(const QVector3D &pointerPosition);
    void released(const QVector3D &pointerPosition);
    void dragged(const QVector3D &pointerPosition);
    void grabsMouseChanged(bool grabsMouse);

protected:
    void classBegin() override {}
    void componentComplete() override;
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    Q_DISABLE_COPY(MouseArea3D)
    QQuick3DViewport *m_view3D = nullptr;

    qreal m_x;
    qreal m_y;
    qreal m_width;
    qreal m_height;

    bool m_hovering = false;
    bool m_dragging = false;

    QVector3D getMousePosInPlane(const QPointF &mousePosInView) const;

    static MouseArea3D *s_mouseGrab;
    bool m_grabsMouse;
};

}
}

QML_DECLARE_TYPE(QmlDesigner::Internal::MouseArea3D)

#endif // QUICK3D_MODULE
