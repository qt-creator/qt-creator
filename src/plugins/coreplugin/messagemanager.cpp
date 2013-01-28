/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#include "messagemanager.h"
#include "messageoutputwindow.h"

#include <extensionsystem/pluginmanager.h>

#include <QStatusBar>
#include <QApplication>

using namespace Core;

MessageManager *MessageManager::m_instance = 0;

MessageManager::MessageManager()
    : m_messageOutputWindow(0)
{
    m_instance = this;
}

MessageManager::~MessageManager()
{
    if (m_messageOutputWindow) {
        ExtensionSystem::PluginManager::removeObject(m_messageOutputWindow);
        delete m_messageOutputWindow;
    }

    m_instance = 0;
}

void MessageManager::init()
{
    m_messageOutputWindow = new Internal::MessageOutputWindow;
    ExtensionSystem::PluginManager::addObject(m_messageOutputWindow);
}

void MessageManager::showOutputPane()
{
    if (m_messageOutputWindow)
        m_messageOutputWindow->popup(IOutputPane::ModeSwitch);
}

void MessageManager::printToOutputPane(const QString &text, bool bringToForeground)
{
    if (!m_messageOutputWindow)
        return;
    if (bringToForeground)
        m_messageOutputWindow->popup(IOutputPane::ModeSwitch);
    m_messageOutputWindow->append(text + QLatin1Char('\n'));
}

void MessageManager::printToOutputPanePopup(const QString &text)
{
    printToOutputPane(text, true);
}

void MessageManager::printToOutputPane(const QString &text)
{
    printToOutputPane(text, false);
}

