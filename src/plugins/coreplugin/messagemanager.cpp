// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "messagemanager.h"

#include "messageoutputwindow.h"

#include <extensionsystem/shutdownguard.h>

#include <utils/qtcassert.h>

#include <QFont>
#include <QPointer>

/*!
    \namespace Core::MessageManager
    \inheaderfile coreplugin/messagemanager.h
    \ingroup mainclasses
    \inmodule QtCreator

    \brief The MessageManager namespace is used to post messages in the
    \uicontrol{General Messages} pane.
*/

using namespace Core::Internal;

namespace Core::MessageManager {

static MessageOutputWindow *messageOutputWindow()
{
    static QPointer<MessageOutputWindow> theMessageOutputWindow
            = new MessageOutputWindow(ExtensionSystem::shutdownGuard());
    return theMessageOutputWindow.get();
}

enum class Flag { Silent, Flash, Disrupt };

static void showOutputPane(Flag flags)
{
    QTC_ASSERT(messageOutputWindow(), return);
    switch (flags) {
    case Flag::Silent:
        break;
    case Flag::Flash:
        messageOutputWindow()->flash();
        break;
    case Flag::Disrupt:
        messageOutputWindow()->popup(IOutputPane::ModeSwitch | IOutputPane::WithFocus);
        break;
    }
}

static void writeImpl(const QString &text, Flag flags)
{
    // Make sure this end up in the GUI thread.
    QMetaObject::invokeMethod(ExtensionSystem::shutdownGuard(), [text, flags] {
        QTC_ASSERT(messageOutputWindow(), return);
        showOutputPane(flags);
        messageOutputWindow()->append(text + '\n');
    });
}

/*!
    \internal
*/
void setFont(const QFont &font)
{
    QTC_ASSERT(messageOutputWindow(), return);
    messageOutputWindow()->setFont(font);
}

/*!
    \internal
*/
void setWheelZoomEnabled(bool enabled)
{
    QTC_ASSERT(messageOutputWindow(), return);
    messageOutputWindow()->setWheelZoomEnabled(enabled);
}

/*!
    Writes the \a message to the \uicontrol{General Messages} pane without
    any further action.

    This is the preferred method of posting messages, since it does not
    interrupt the user.

    \sa writeFlashing()
    \sa writeDisrupting()
*/
void writeSilently(const QString &message)
{
    writeImpl(message, Flag::Silent);
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
void writeFlashing(const QString &message)
{
    writeImpl(message, Flag::Flash);
}

/*!
    Writes the \a message to the \uicontrol{General Messages} pane and brings
    the pane to the front.

    This might interrupt a user's workflow, so only use this as a direct
    response to something a user did, like explicitly running a tool.

    \sa writeSilently()
    \sa writeFlashing()
*/
void writeDisrupting(const QString &message)
{
    writeImpl(message, Flag::Disrupt);
}

/*!
    \overload writeSilently()
*/
void writeSilently(const QStringList &messages)
{
    writeSilently(messages.join('\n'));
}

/*!
    \overload writeFlashing()
*/
void writeFlashing(const QStringList &messages)
{
    writeFlashing(messages.join('\n'));
}

/*!
    \overload writeDisrupting()
*/
void writeDisrupting(const QStringList &messages)
{
    writeDisrupting(messages.join('\n'));
}

} // namespace Core::MessageManager
