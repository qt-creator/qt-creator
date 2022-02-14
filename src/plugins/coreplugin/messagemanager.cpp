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

/*!
    \class Core::MessageManager
    \inheaderfile coreplugin/messagemanager.h
    \ingroup mainclasses
    \inmodule QtCreator

    \brief The MessageManager class is used to post messages in the
    \uicontrol{General Messages} pane.
*/

namespace Core {

static MessageManager *m_instance = nullptr;
static Internal::MessageOutputWindow *m_messageOutputWindow = nullptr;

/*!
    \internal
*/
MessageManager *MessageManager::instance()
{
    return m_instance;
}

enum class Flag { Silent, Flash, Disrupt };

static void showOutputPane(Flag flags)
{
    QTC_ASSERT(m_messageOutputWindow, return);

    switch (flags) {
    case Core::Flag::Silent:
        break;
    case Core::Flag::Flash:
        m_messageOutputWindow->flash();
        break;
    case Core::Flag::Disrupt:
        m_messageOutputWindow->popup(IOutputPane::ModeSwitch | IOutputPane::WithFocus);
        break;
    }
}

static void doWrite(const QString &text, Flag flags)
{
    QTC_ASSERT(m_messageOutputWindow, return);

    showOutputPane(flags);
    m_messageOutputWindow->append(text + '\n');
}

static void write(const QString &text, Flag flags)
{
    QTC_ASSERT(m_instance, return);
    if (QThread::currentThread() == m_instance->thread())
        doWrite(text, flags);
    else
        QMetaObject::invokeMethod(m_instance, [text, flags] {
            doWrite(text, flags);
        }, Qt::QueuedConnection);
}

/*!
    \internal
*/
MessageManager::MessageManager()
{
    m_instance = this;
    m_messageOutputWindow = nullptr;
}

/*!
    \internal
*/
MessageManager::~MessageManager()
{
    if (m_messageOutputWindow) {
        ExtensionSystem::PluginManager::removeObject(m_messageOutputWindow);
        delete m_messageOutputWindow;
    }
    m_instance = nullptr;
}

/*!
    \internal
*/
void MessageManager::init()
{
    m_messageOutputWindow = new Internal::MessageOutputWindow;
    ExtensionSystem::PluginManager::addObject(m_messageOutputWindow);
}

/*!
    \internal
*/
void MessageManager::setFont(const QFont &font)
{
    QTC_ASSERT(m_messageOutputWindow, return);

    m_messageOutputWindow->setFont(font);
}

/*!
    \internal
*/
void MessageManager::setWheelZoomEnabled(bool enabled)
{
    QTC_ASSERT(m_messageOutputWindow, return);

    m_messageOutputWindow->setWheelZoomEnabled(enabled);
}

/*!
    Writes the \a message to the \uicontrol{General Messages} pane without
    any further action.

    This is the preferred method of posting messages, since it does not
    interrupt the user.

    \sa writeFlashing()
    \sa writeDisrupting()
*/
void MessageManager::writeSilently(const QString &message)
{
    Core::write(message, Flag::Silent);
}

/*!
    Writes the \a message to the \uicontrol{General Messages} pane and flashes
    the output pane button.

    This notifies the user that something important has happened that might
    require the user's attention. Use sparingly, since continually flashing the
    button is annoying, especially if the condition is something the user might
    not be able to fix.

    \sa writeSilently()
    \sa writeDisrupting()
*/
void MessageManager::writeFlashing(const QString &message)
{
    Core::write(message, Flag::Flash);
}

/*!
    Writes the \a message to the \uicontrol{General Messages} pane and brings
    the pane to the front.

    This might interrupt a user's workflow, so only use this as a direct
    response to something a user did, like explicitly running a tool.

    \sa writeSilently()
    \sa writeFlashing()
*/
void MessageManager::writeDisrupting(const QString &message)
{
    Core::write(message, Flag::Disrupt);
}

/*!
    \overload writeSilently()
*/
void MessageManager::writeSilently(const QStringList &messages)
{
    writeSilently(messages.join('\n'));
}

/*!
    \overload writeFlashing()
*/
void MessageManager::writeFlashing(const QStringList &messages)
{
    writeFlashing(messages.join('\n'));
}

/*!
    \overload writeDisrupting()
*/
void MessageManager::writeDisrupting(const QStringList &messages)
{
    writeDisrupting(messages.join('\n'));
}

} // namespace Core
