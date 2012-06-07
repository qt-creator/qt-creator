/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

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
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    if (pm && m_messageOutputWindow) {
        pm->removeObject(m_messageOutputWindow);
        delete m_messageOutputWindow;
    }

    m_instance = 0;
}

void MessageManager::init()
{
    m_messageOutputWindow = new Internal::MessageOutputWindow;
    ExtensionSystem::PluginManager::instance()->addObject(m_messageOutputWindow);
}

void MessageManager::showOutputPane()
{
    if (m_messageOutputWindow)
        m_messageOutputWindow->popup(false);
}

void MessageManager::printToOutputPane(const QString &text, bool bringToForeground)
{
    if (!m_messageOutputWindow)
        return;
    if (bringToForeground)
        m_messageOutputWindow->popup(false);
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

