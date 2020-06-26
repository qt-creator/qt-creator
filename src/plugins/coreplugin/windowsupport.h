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
