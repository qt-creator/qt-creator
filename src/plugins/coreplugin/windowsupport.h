// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "icontext.h"

#include <QObject>

QT_BEGIN_NAMESPACE
class QAction;
class QMenu;
class QWidget;
QT_END_NAMESPACE

namespace Core {
namespace Internal {

class WindowList
{
public:
    ~WindowList();

    void addWindow(QWidget *window);
    void removeWindow(QWidget *window);
    void setActiveWindow(QWidget *window);
    void setWindowVisible(QWidget *window, bool visible);

private:
    void activateWindow(QAction *action);
    void updateTitle(QWidget *window);

    QMenu *m_dockMenu = nullptr;
    QList<QWidget *> m_windows;
    QList<QAction *> m_windowActions;
    QList<Utils::Id> m_windowActionIds;
};

class WindowSupport : public QObject
{
    Q_OBJECT
public:
    WindowSupport(QWidget *window, const Context &context);
    ~WindowSupport() override;

    void setCloseActionEnabled(bool enabled);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    void toggleFullScreen();
    void updateFullScreenAction();

    QWidget *m_window;
    IContext *m_contextObject;
    QAction *m_minimizeAction;
    QAction *m_zoomAction;
    QAction *m_closeAction;
    QAction *m_toggleFullScreenAction;
    Qt::WindowStates m_previousWindowState;
    bool m_shutdown = false;
};

} // Internal
} // Core
