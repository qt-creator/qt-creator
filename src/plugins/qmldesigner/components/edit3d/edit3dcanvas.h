// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#pragma once

#include <QtWidgets/qwidget.h>
#include <QtGui/qimage.h>
#include <QtGui/qevent.h>
#include <QtCore/qelapsedtimer.h>
#include <QtCore/qpointer.h>

namespace QmlDesigner {

class Edit3DWidget;

class Edit3DCanvas : public QWidget
{
    Q_OBJECT

public:
    Edit3DCanvas(Edit3DWidget *parent);

    void updateRenderImage(const QImage &img);
    void updateActiveScene(qint32 activeScene);
    qint32 activeScene() const { return m_activeScene; }
    QImage renderImage() const;
    void setOpacity(qreal opacity);
    QWidget *busyIndicator() const;

protected:
    void mousePressEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;
    void mouseDoubleClickEvent(QMouseEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;
    void wheelEvent(QWheelEvent *e) override;
    void keyPressEvent(QKeyEvent *e) override;
    void keyReleaseEvent(QKeyEvent *e) override;
    void paintEvent(QPaintEvent *e) override;
    void resizeEvent(QResizeEvent *e) override;
    void focusOutEvent(QFocusEvent *focusEvent) override;
    void focusInEvent(QFocusEvent *focusEvent) override;

private:
    void positionBusyInidicator();

    QPointer<Edit3DWidget> m_parent;
    QImage m_image;
    qint32 m_activeScene = -1;
    QElapsedTimer m_usageTimer;
    qreal m_opacity = 1.0;
    QWidget *m_busyIndicator = nullptr;
};

} // namespace QmlDesigner
