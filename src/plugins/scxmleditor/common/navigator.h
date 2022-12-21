// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
