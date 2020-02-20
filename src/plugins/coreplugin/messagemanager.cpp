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

#include "messagemanager.h"

#include "messageoutputwindow.h"

#include <extensionsystem/pluginmanager.h>
#include <utils/qtcassert.h>

#include <QFont>
#include <QThread>
#include <QTime>
#include <QTimer>

using namespace Core;

static MessageManager *m_instance = nullptr;
static Internal::MessageOutputWindow *m_messageOutputWindow = nullptr;

MessageManager *MessageManager::instance()
{
    return m_instance;
}

void MessageManager::showOutputPane(Core::MessageManager::PrintToOutputPaneFlags flags)
{
    QTC_ASSERT(m_messageOutputWindow, return);

    if (flags & Flash) {
        m_messageOutputWindow->flash();
    } else if (flags & Silent) {
        // Do nothing
    } else {
        m_messageOutputWindow->popup(IOutputPane::Flag(int(flags)));
    }
}

MessageManager::MessageManager()
{
    m_instance = this;
    m_messageOutputWindow = nullptr;
    qRegisterMetaType<MessageManager::PrintToOutputPaneFlags>();
}

MessageManager::~MessageManager()
{
    if (m_messageOutputWindow) {
        ExtensionSystem::PluginManager::removeObject(m_messageOutputWindow);
        delete m_messageOutputWindow;
    }
    m_instance = nullptr;
}

void MessageManager::init()
{
    m_messageOutputWindow = new Internal::MessageOutputWindow;
    ExtensionSystem::PluginManager::addObject(m_messageOutputWindow);
}

void MessageManager::setFont(const QFont &font)
{
    QTC_ASSERT(m_messageOutputWindow, return);

    m_messageOutputWindow->setFont(font);
}

void MessageManager::setWheelZoomEnabled(bool enabled)
{
    QTC_ASSERT(m_messageOutputWindow, return);

    m_messageOutputWindow->setWheelZoomEnabled(enabled);
}

void MessageManager::writeMessages(const QStringList &messages, PrintToOutputPaneFlags flags)
{
    write(messages.join('\n'), flags);
}

void MessageManager::write(const QString &text, PrintToOutputPaneFlags flags)
{
    if (QThread::currentThread() == instance()->thread())
        doWrite(text, flags);
    else
        QTimer::singleShot(0, instance(), [text, flags] { doWrite(text, flags); });
}

void MessageManager::writeWithTime(const QString &text, PrintToOutputPaneFlags flags)
{
    const QString timeStamp = QTime::currentTime().toString("HH:mm:ss ");
    write(timeStamp + text, flags);
}

void MessageManager::doWrite(const QString &text, PrintToOutputPaneFlags flags)
{
    QTC_ASSERT(m_messageOutputWindow, return);

    showOutputPane(flags);
    m_messageOutputWindow->append(text + '\n');
}
