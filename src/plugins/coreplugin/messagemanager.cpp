/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
** 
** Non-Open Source Usage  
** 
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.  
** 
** GNU General Public License Usage 
** 
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception version
** 1.2, included in the file GPL_EXCEPTION.txt in this package.  
** 
***************************************************************************/

#include "messagemanager.h"
#include "messageoutputwindow.h"

#include <extensionsystem/pluginmanager.h>

#include <QtGui/QStatusBar>
#include <QtGui/QApplication>

using namespace Core;

MessageManager *MessageManager::m_instance = 0;

MessageManager::MessageManager()
    : m_pm(0), m_messageOutputWindow(0)
{
    m_instance = this;
}

MessageManager::~MessageManager()
{
    if (m_pm && m_messageOutputWindow) {
        m_pm->removeObject(m_messageOutputWindow);
        delete m_messageOutputWindow;
    }

    m_instance = 0;
}

void MessageManager::init(ExtensionSystem::PluginManager *pm)
{
    m_pm = pm;
    m_messageOutputWindow = new Internal::MessageOutputWindow;
    pm->addObject(m_messageOutputWindow);
}

void MessageManager::showOutputPane()
{
    if (m_messageOutputWindow)
        m_messageOutputWindow->popup(false);
}

void MessageManager::displayStatusBarMessage(const QString & /*text*/, int /*ms*/)
{
    // TODO: Currently broken, but noone really notices, so...
    //m_mainWindow->statusBar()->showMessage(text, ms);
}

void MessageManager::printToOutputPane(const QString &text, bool bringToForeground)
{
    if (!m_messageOutputWindow)
        return;
    if (bringToForeground)
        m_messageOutputWindow->popup(false);
    m_messageOutputWindow->append(text);
}
