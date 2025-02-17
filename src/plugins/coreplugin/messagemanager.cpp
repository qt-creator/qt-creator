// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "messagemanager.h"

#include "coreconstants.h"
#include "coreplugintr.h"
#include "icontext.h"
#include "ioutputpane.h"
#include "outputwindow.h"

#include <utils/qtcassert.h>
#include <utils/shutdownguard.h>
#include <utils/utilsicons.h>

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

const char zoomSettingsKey[] = "Core/MessageOutput/Zoom";

class MessageOutputWindow final : public IOutputPane
{
public:
    explicit MessageOutputWindow(QObject *parent)
        : IOutputPane(parent)
    {
        setId("GeneralMessages");
        setDisplayName(Tr::tr("General Messages"));
        setPriorityInStatusBar(-100);

        m_widget = new OutputWindow(Context(Constants::C_GENERAL_OUTPUT_PANE), zoomSettingsKey);
        m_widget->setReadOnly(true);

        connect(this, &IOutputPane::zoomInRequested, m_widget, &Core::OutputWindow::zoomIn);
        connect(this, &IOutputPane::zoomOutRequested, m_widget, &Core::OutputWindow::zoomOut);
        connect(this, &IOutputPane::resetZoomRequested, m_widget, &Core::OutputWindow::resetZoom);
        connect(this, &IOutputPane::fontChanged, m_widget, &OutputWindow::setBaseFont);
        connect(this, &IOutputPane::wheelZoomEnabledChanged, m_widget, &OutputWindow::setWheelZoomEnabled);

        setupFilterUi("MessageOutputPane.Filter", "Core::Internal::MessageOutputWindow");
        setFilteringEnabled(true);
        setupContext(Constants::C_GENERAL_OUTPUT_PANE, m_widget);
    }

    ~MessageOutputWindow() final
    {
        delete m_widget;
    }

    void append(const QString &text)
    {
        m_widget->appendMessage(text, Utils::GeneralMessageFormat);
    }

private:
    QWidget *outputWidget(QWidget *parent) final
    {
        m_widget->setParent(parent);
        return m_widget;
    }

    void clearContents() final { m_widget->clear(); }

    bool canFocus() const final { return true; }
    bool hasFocus() const final { return m_widget->window()->focusWidget() == m_widget; }
    void setFocus() final { m_widget->setFocus(); }

    bool canNext() const final { return false; }
    bool canPrevious() const final { return false; }
    void goToNext() final {}
    void goToPrev() final {}
    bool canNavigate() const final { return false; }

    bool hasFilterContext() const final { return true; }

    void updateFilter() final
    {
        m_widget->updateFilterProperties(filterText(), filterCaseSensitivity(), filterUsesRegexp(),
                                         filterIsInverted(), beforeContext(), afterContext());
    }

    OutputWindow *m_widget = nullptr;
};

static MessageOutputWindow *messageOutputWindow()
{
    static QPointer<MessageOutputWindow> theMessageOutputWindow
            = new MessageOutputWindow(Utils::shutdownGuard());
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
    QMetaObject::invokeMethod(Utils::shutdownGuard(), [text, flags] {
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
