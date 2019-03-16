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

#include "messageoutputwindow.h"
#include "outputwindow.h"
#include "icontext.h"
#include "coreconstants.h"
#include "find/basetextfind.h"

#include <aggregation/aggregate.h>
#include <coreplugin/icore.h>
#include <utils/utilsicons.h>

#include <QFont>
#include <QToolButton>

namespace Core {
namespace Internal {

const char zoomSettingsKey[] = "Core/MessageOutput/Zoom";

MessageOutputWindow::MessageOutputWindow() :
    m_zoomInButton(new QToolButton),
    m_zoomOutButton(new QToolButton)
{
    m_widget = new OutputWindow(Context(Constants::C_GENERAL_OUTPUT_PANE));
    m_widget->setReadOnly(true);
    // Let selected text be colored as if the text edit was editable,
    // otherwise the highlight for searching is too light
    QPalette p = m_widget->palette();
    QColor activeHighlight = p.color(QPalette::Active, QPalette::Highlight);
    p.setColor(QPalette::Highlight, activeHighlight);
    QColor activeHighlightedText = p.color(QPalette::Active, QPalette::HighlightedText);
    p.setColor(QPalette::HighlightedText, activeHighlightedText);
    m_widget->setPalette(p);
    auto agg = new Aggregation::Aggregate;
    agg->add(m_widget);
    agg->add(new BaseTextFind(m_widget));

    loadSettings();

    m_zoomInButton->setToolTip(tr("Increase Font Size"));
    m_zoomInButton->setIcon(Utils::Icons::PLUS_TOOLBAR.icon());
    m_zoomInButton->setAutoRaise(true);
    connect(m_zoomInButton, &QToolButton::clicked, this, [this] { m_widget->zoomIn(1); });

    m_zoomOutButton->setToolTip(tr("Decrease Font Size"));
    m_zoomOutButton->setIcon(Utils::Icons::MINUS.icon());
    m_zoomOutButton->setAutoRaise(true);
    connect(m_zoomOutButton, &QToolButton::clicked, this, [this] { m_widget->zoomOut(1); });

    connect(Core::ICore::instance(), &Core::ICore::saveSettingsRequested,
            this, &MessageOutputWindow::storeSettings);
}

MessageOutputWindow::~MessageOutputWindow()
{
    delete m_zoomInButton;
    delete m_zoomOutButton;
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

QList<QWidget *> MessageOutputWindow::toolBarWidgets() const
{
    return {m_zoomInButton, m_zoomOutButton};
}

QString MessageOutputWindow::displayName() const
{
    return tr("General Messages");
}

void MessageOutputWindow::visibilityChanged(bool /*b*/)
{
}

void MessageOutputWindow::append(const QString &text)
{
    m_widget->appendText(text);
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

void MessageOutputWindow::setFont(const QFont &font)
{
    m_widget->setBaseFont(font);
}

void MessageOutputWindow::setWheelZoomEnabled(bool enabled)
{
    m_widget->setWheelZoomEnabled(enabled);
}

void MessageOutputWindow::storeSettings() const
{
    QSettings * const s = Core::ICore::settings();

    s->setValue(zoomSettingsKey, m_widget->fontZoom());
}

void MessageOutputWindow::loadSettings()
{
    QSettings * const s = Core::ICore::settings();

    float zoom = s->value(zoomSettingsKey, 0).toFloat();
    m_widget->setFontZoom(zoom);
}

} // namespace Internal
} // namespace Core
