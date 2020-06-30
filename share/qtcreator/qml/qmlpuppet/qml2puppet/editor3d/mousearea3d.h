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
#include <QtCore/qpoint.h>
#include <QtCore/qpointer.h>

#include <QtQml/qqmlparserstatus.h>

#include <QtQuick3D/private/qquick3dnode_p.h>
#include <QtQuick3D/private/qquick3dmodel_p.h>
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
    Q_PROPERTY(int priority READ priority WRITE setPriority NOTIFY priorityChanged)
    Q_PROPERTY(bool active READ active WRITE setActive NOTIFY activeChanged)
    Q_PROPERTY(QPointF circlePickArea READ circlePickArea WRITE setCirclePickArea NOTIFY circlePickAreaChanged)
    Q_PROPERTY(qreal minAngle READ minAngle WRITE setMinAngle NOTIFY minAngleChanged)
    Q_PROPERTY(QQuick3DNode *pickNode READ pickNode WRITE setPickNode NOTIFY pickNodeChanged)
    Q_PROPERTY(MouseArea3D *dragHelper READ dragHelper WRITE setDragHelper NOTIFY dragHelperChanged)

    Q_INTERFACES(QQmlParserStatus)

public:
    MouseArea3D(QQuick3DNode *parent = nullptr);

    QQuick3DViewport *view3D() const;

    qreal x() const;
    qreal y() const;
    qreal width() const;
    qreal height() const;
    int priority() const;

    bool hovering() const;
    bool dragging() const;
    bool grabsMouse() const;
    bool active() const;
    QPointF circlePickArea() const;
    qreal minAngle() const;
    QQuick3DNode *pickNode() const;
    MouseArea3D *dragHelper() const;

    static qreal mouseDragMultiplier() { return .02; }

public slots:
    void setView3D(QQuick3DViewport *view3D);
    void setGrabsMouse(bool grabsMouse);
    void setActive(bool active);
    void setCirclePickArea(const QPointF &pickArea);
    void setMinAngle(qreal angle);
    void setPickNode(QQuick3DNode *node);
    void setDragHelper(MouseArea3D *dragHelper);

    void setX(qreal x);
    void setY(qreal y);
    void setWidth(qreal width);
    void setHeight(qreal height);
    void setPriority(int level);

    Q_INVOKABLE QVector3D rayIntersectsPlane(const QVector3D &rayPos0,
                                             const QVector3D &rayPos1,
                                             const QVector3D &planePos,
                                             const QVector3D &planeNormal) const;

    Q_INVOKABLE QVector3D getNewScale(const QVector3D &startScale, const QVector2D &relativeDistance,
                                      const QVector3D &axisX, const QVector3D &axisY);

    Q_INVOKABLE qreal getNewRotationAngle(QQuick3DNode *node, const QVector3D &pressPos,
                                          const QVector3D &currentPos, const QVector3D &nodePos,
                                          qreal prevAngle, bool trackBall);
    Q_INVOKABLE void applyRotationAngleToNode(QQuick3DNode *node, const QVector3D &startRotation,
                                              qreal angle);
    Q_INVOKABLE void applyFreeRotation(QQuick3DNode *node, const QVector3D &startRotation,
                                       const QVector3D &pressPos, const QVector3D &currentPos);
    Q_INVOKABLE QVector3D pivotScenePosition(QQuick3DNode *node) const;
    Q_INVOKABLE double getRelativeScale(QQuick3DNode *node) const;

    Q_INVOKABLE void forcePressEvent(double x, double y);
    Q_INVOKABLE void forceMoveEvent(double x, double y);
    Q_INVOKABLE void forceReleaseEvent(double x, double y);

signals:
    void view3DChanged();

    void xChanged();
    void yChanged();
    void widthChanged();
    void heightChanged();
    void priorityChanged();

    void hoveringChanged();
    void draggingChanged();
    void activeChanged();
    void grabsMouseChanged();
    void circlePickAreaChanged();
    void minAngleChanged();
    void pickNodeChanged();
    void dragHelperChanged();

    // angle parameter is only set if circlePickArea is specified
    void pressed(const QVector2D &planePos, const QPoint &screenPos, qreal angle);
    void released(const QVector2D &planePos, const QPoint &screenPos);
    void dragged(const QVector2D &planePos, const QPoint &screenPos);

protected:
    void classBegin() override {}
    void componentComplete() override;
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    void setDragging(bool enable);
    void setHovering(bool enable);
    QVector3D getNormal() const;
    QVector3D getCameraToNodeDir(QQuick3DNode *node) const;
    QVector3D getMousePosInPlane(const MouseArea3D *helper, const QPointF &mousePosInView) const;

    Q_DISABLE_COPY(MouseArea3D)
    QQuick3DViewport *m_view3D = nullptr;

    qreal m_x;
    qreal m_y;
    qreal m_width;
    qreal m_height;
    int m_priority = 0;

    bool m_hovering = false;
    bool m_dragging = false;
    bool m_active = false;

    static MouseArea3D *s_mouseGrab;
    bool m_grabsMouse = false;
    QVector3D m_mousePosInPlane;
    QPointF m_circlePickArea;
    qreal m_minAngle = 0.;
    QQuick3DNode *m_pickNode = nullptr;
    MouseArea3D *m_dragHelper = nullptr;
    bool m_forceCaptureNextPress = false;
};

}
}

QML_DECLARE_TYPE(QmlDesigner::Internal::MouseArea3D)

#endif // QUICK3D_MODULE
