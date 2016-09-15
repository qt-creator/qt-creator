/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include <QGraphicsView>
#include <QPointer>

#include "ui_magnifier.h"

QT_FORWARD_DECLARE_CLASS(QMouseEvent)

namespace ScxmlEditor {

namespace PluginInterface { class GraphicsScene; }

namespace Common {

class GraphicsView;

class Magnifier : public QWidget
{
    Q_OBJECT

public:
    Magnifier(QWidget *parent = nullptr);

    void setCurrentView(GraphicsView *mainView);
    void setCurrentScene(PluginInterface::GraphicsScene *scene);
    void setTopLeft(const QPoint &topLeft);

signals:
    void clicked(double zoomLevel);

protected:
    void resizeEvent(QResizeEvent *e) override;
    void paintEvent(QPaintEvent *e) override;
    void showEvent(QShowEvent *e) override;
    void hideEvent(QHideEvent *e) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void moveEvent(QMoveEvent *e) override;
    void wheelEvent(QWheelEvent *e) override;

private:
    QPoint m_topLeft;
    QPointer<GraphicsView> m_mainView;
    QRadialGradient m_gradientBrush;
    Ui::Magnifier m_ui;
};

} // namespace Common
} // namespace ScxmlEditor
