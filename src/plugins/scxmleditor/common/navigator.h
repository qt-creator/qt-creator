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

#include "movableframe.h"
#include <QPointer>

QT_FORWARD_DECLARE_CLASS(QToolButton)

namespace ScxmlEditor {

namespace PluginInterface { class GraphicsScene; }

namespace Common {

class NavigatorGraphicsView;
class NavigatorSlider;
class SizeGrip;
class GraphicsView;

/**
 * @brief The Navigator class is the "minimap" widget for navigate and zoom in the scene.
 */
class Navigator : public MovableFrame
{
    Q_OBJECT

public:
    explicit Navigator(QWidget *parent = nullptr);

    void setCurrentView(GraphicsView *view);
    void setCurrentScene(PluginInterface::GraphicsScene *scene);

protected:
    void resizeEvent(QResizeEvent *e) override;

private:
    void createUi();

    QPointer<GraphicsView> m_currentView;
    NavigatorGraphicsView *m_navigatorView = nullptr;
    NavigatorSlider *m_navigatorSlider = nullptr;
    QToolButton *m_closeButton = nullptr;
    SizeGrip *m_sizeGrip;
};

} // namespace Common
} // namespace ScxmlEditor
