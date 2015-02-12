/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "messagemanager.h"
#include "messageoutputwindow.h"

#include <extensionsystem/pluginmanager.h>

using namespace Core;

static MessageManager *m_instance = 0;
Internal::MessageOutputWindow *m_messageOutputWindow = 0;

QObject *MessageManager::instance()
{
    return m_instance;
}

MessageManager::MessageManager()
{
    m_instance = this;
    m_messageOutputWindow = 0;
    qRegisterMetaType<MessageManager::PrintToOutputPaneFlags>();
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

void MessageManager::write(const QString &text)
{
    write(text, NoModeSwitch);
}

void MessageManager::write(const QString &text, PrintToOutputPaneFlags flags)
{
    if (!m_messageOutputWindow)
        return;
    if (flags & Flash) {
        m_messageOutputWindow->flash();
    } else if (flags & Silent) {
        // Do nothing
    } else {
        m_messageOutputWindow->popup(IOutputPane::Flag(int(flags)));
    }

    m_messageOutputWindow->append(text + QLatin1Char('\n'));
}

