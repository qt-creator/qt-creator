/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "windowsupport.h"

#include "actionmanager/actionmanager.h"
#include "coreconstants.h"
#include "icore.h"

#include <utils/hostosinfo.h>

#include <QAction>
#include <QWidget>
#include <QEvent>

namespace Core {
namespace Internal {

WindowSupport::WindowSupport(QWidget *window, const Context &context)
    : QObject(window),
      m_window(window)
{
    m_window->installEventFilter(this);

    m_contextObject = new IContext(this);
    m_contextObject->setWidget(window);
    m_contextObject->setContext(context);
    ICore::addContextObject(m_contextObject);

    if (UseMacShortcuts) {
        m_minimizeAction = new QAction(this);
        ActionManager::registerAction(m_minimizeAction, Constants::MINIMIZE_WINDOW, context);
        connect(m_minimizeAction, SIGNAL(triggered()), m_window, SLOT(showMinimized()));

        m_zoomAction = new QAction(this);
        ActionManager::registerAction(m_zoomAction, Constants::ZOOM_WINDOW, context);
        connect(m_zoomAction, SIGNAL(triggered()), m_window, SLOT(showMaximized()));

        m_closeAction = new QAction(this);
        ActionManager::registerAction(m_closeAction, Constants::CLOSE_WINDOW, context);
        connect(m_closeAction, SIGNAL(triggered()), m_window, SLOT(close()), Qt::QueuedConnection);
    }

    m_toggleFullScreenAction = new QAction(this);
    updateFullScreenAction();
    ActionManager::registerAction(m_toggleFullScreenAction, Constants::TOGGLE_FULLSCREEN, context);
    connect(m_toggleFullScreenAction, SIGNAL(triggered()), this, SLOT(toggleFullScreen()));
}

WindowSupport::~WindowSupport()
{
    if (UseMacShortcuts) {
        ActionManager::unregisterAction(m_minimizeAction, Constants::MINIMIZE_WINDOW);
        ActionManager::unregisterAction(m_zoomAction, Constants::ZOOM_WINDOW);
        ActionManager::unregisterAction(m_closeAction, Constants::CLOSE_WINDOW);
    }
    ActionManager::unregisterAction(m_toggleFullScreenAction, Constants::TOGGLE_FULLSCREEN);
    ICore::removeContextObject(m_contextObject);
}

void WindowSupport::setCloseActionEnabled(bool enabled)
{
    if (UseMacShortcuts)
        m_closeAction->setEnabled(enabled);
}

bool WindowSupport::eventFilter(QObject *obj, QEvent *event)
{
    if (obj != m_window)
        return false;
    if (event->type() == QEvent::WindowStateChange) {
        if (Utils::HostOsInfo::isMacHost()) {
            bool minimized = m_window->isMinimized();
            m_minimizeAction->setEnabled(!minimized);
            m_zoomAction->setEnabled(!minimized);
        }
        updateFullScreenAction();
    }
    return false;
}

void WindowSupport::toggleFullScreen()
{
    if (m_window->isFullScreen()) {
        m_window->setWindowState(m_window->windowState() & ~Qt::WindowFullScreen);
    } else {
        m_window->setWindowState(m_window->windowState() | Qt::WindowFullScreen);
    }
}

void WindowSupport::updateFullScreenAction()
{
    if (m_window->isFullScreen()) {
        if (Utils::HostOsInfo::isMacHost())
            m_toggleFullScreenAction->setText(tr("Exit Full Screen"));
        else
            m_toggleFullScreenAction->setChecked(true);
    } else {
        if (Utils::HostOsInfo::isMacHost())
            m_toggleFullScreenAction->setText(tr("Enter Full Screen"));
        else
            m_toggleFullScreenAction->setChecked(false);
    }
}


} // Internal
} // Core
