// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "messagemanager.h"

#include "messageoutputwindow.h"

#include <utils/qtcassert.h>

#include <QFont>

#include <memory>

/*!
    \namespace Core::MessageManager
    \inheaderfile coreplugin/messagemanager.h
    \ingroup mainclasses
    \inmodule QtCreator

    \brief The MessageManager namespace is used to post messages in the
    \uicontrol{General Messages} pane.
*/

namespace Core::MessageManager {

static std::unique_ptr<Internal::MessageOutputWindow> s_messageOutputWindow;

enum class Flag { Silent, Flash, Disrupt };

static void showOutputPane(Flag flags)
{
    QTC_ASSERT(s_messageOutputWindow, return);
    switch (flags) {
    case Flag::Silent:
        break;
    case Flag::Flash:
        s_messageOutputWindow->flash();
        break;
    case Flag::Disrupt:
        s_messageOutputWindow->popup(IOutputPane::ModeSwitch | IOutputPane::WithFocus);
        break;
    }
}

static void doWrite(const QString &text, Flag flags)
{
    QTC_ASSERT(s_messageOutputWindow, return);
    showOutputPane(flags);
    s_messageOutputWindow->append(text + '\n');
}

static void writeImpl(const QString &text, Flag flags)
{
    QTC_ASSERT(s_messageOutputWindow, return);
    QMetaObject::invokeMethod(s_messageOutputWindow.get(), [text, flags] { doWrite(text, flags); });
}

/*!
    \internal
*/
void init()
{
    s_messageOutputWindow.reset(new Internal::MessageOutputWindow);
}

/*!
    \internal
*/
void destroy()
{
    s_messageOutputWindow.reset();
}

/*!
    \internal
*/
void setFont(const QFont &font)
{
    QTC_ASSERT(s_messageOutputWindow, return);
    s_messageOutputWindow->setFont(font);
}

/*!
    \internal
*/
void setWheelZoomEnabled(bool enabled)
{
    QTC_ASSERT(s_messageOutputWindow, return);
    s_messageOutputWindow->setWheelZoomEnabled(enabled);
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
