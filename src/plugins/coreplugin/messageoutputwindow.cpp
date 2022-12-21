// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "messageoutputwindow.h"
#include "outputwindow.h"
#include "icontext.h"
#include "coreconstants.h"

#include <coreplugin/icore.h>
#include <utils/utilsicons.h>

#include <QFont>
#include <QToolButton>

namespace Core {
namespace Internal {

const char zoomSettingsKey[] = "Core/MessageOutput/Zoom";

MessageOutputWindow::MessageOutputWindow()
{
    m_widget = new OutputWindow(Context(Constants::C_GENERAL_OUTPUT_PANE), zoomSettingsKey);
    m_widget->setReadOnly(true);

    connect(this, &IOutputPane::zoomInRequested, m_widget, &Core::OutputWindow::zoomIn);
    connect(this, &IOutputPane::zoomOutRequested, m_widget, &Core::OutputWindow::zoomOut);
    connect(this, &IOutputPane::resetZoomRequested, m_widget, &Core::OutputWindow::resetZoom);
    connect(this, &IOutputPane::fontChanged, m_widget, &OutputWindow::setBaseFont);
    connect(this, &IOutputPane::wheelZoomEnabledChanged, m_widget, &OutputWindow::setWheelZoomEnabled);

    setupFilterUi("MessageOutputPane.Filter");
    setFilteringEnabled(true);
    setupContext(Constants::C_GENERAL_OUTPUT_PANE, m_widget);
}

MessageOutputWindow::~MessageOutputWindow()
{
    delete m_widget;
}

bool MessageOutputWindow::hasFocus() const
{
    return m_widget->window()->focusWidget() == m_widget;
}

bool MessageOutputWindow::canFocus() const
{
    return true;
}

void MessageOutputWindow::setFocus()
{
    m_widget->setFocus();
}

void MessageOutputWindow::clearContents()
{
    m_widget->clear();
}

QWidget *MessageOutputWindow::outputWidget(QWidget *parent)
{
    m_widget->setParent(parent);
    return m_widget;
}

QString MessageOutputWindow::displayName() const
{
    return tr("General Messages");
}

void MessageOutputWindow::append(const QString &text)
{
    m_widget->appendMessage(text, Utils::GeneralMessageFormat);
}

int MessageOutputWindow::priorityInStatusBar() const
{
    return -1;
}

bool MessageOutputWindow::canNext() const
{
    return false;
}

bool MessageOutputWindow::canPrevious() const
{
    return false;
}

void MessageOutputWindow::goToNext()
{

}

void MessageOutputWindow::goToPrev()
{

}

bool MessageOutputWindow::canNavigate() const
{
    return false;
}

void MessageOutputWindow::updateFilter()
{
    m_widget->updateFilterProperties(filterText(), filterCaseSensitivity(), filterUsesRegexp(),
                                     filterIsInverted());
}

} // namespace Internal
} // namespace Core
