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

#include "basehoverhandler.h"
#include "texteditor.h"

#include <utils/executeondestruction.h>
#include <utils/qtcassert.h>
#include <utils/tooltip/tooltip.h>

#include <QVBoxLayout>

namespace TextEditor {

BaseHoverHandler::~BaseHoverHandler() = default;

void BaseHoverHandler::showToolTip(TextEditorWidget *widget, const QPoint &point)
{
    operateTooltip(widget, point);
}

void BaseHoverHandler::checkPriority(TextEditorWidget *widget,
                                     int pos,
                                     ReportPriority report)
{
    widget->setContextHelpItem({});

    process(widget, pos, report);
}

int BaseHoverHandler::priority() const
{
    if (m_priority >= 0)
        return m_priority;

    if (lastHelpItemIdentified().isValid())
        return Priority_Help;

    if (!toolTip().isEmpty())
        return Priority_Tooltip;

    return Priority_None;
}

void BaseHoverHandler::setPriority(int priority)
{
    m_priority = priority;
}

void BaseHoverHandler::contextHelpId(TextEditorWidget *widget,
                                     int pos,
                                     const Core::IContext::HelpCallback &callback)
{
    m_isContextHelpRequest = true;

    // If the tooltip is visible and there is a help match, this match is used to update
    // the help id. Otherwise, let the identification process happen.
    if (!Utils::ToolTip::isVisible() || !lastHelpItemIdentified().isValid()) {
        process(widget, pos, [this, widget = QPointer<TextEditorWidget>(widget), callback](int) {
            if (widget)
                propagateHelpId(widget, callback);
        });
    } else {
        propagateHelpId(widget, callback);
    }

    m_isContextHelpRequest = false;
}

void BaseHoverHandler::setToolTip(const QString &tooltip, Qt::TextFormat format)
{
    m_toolTip = tooltip;
    m_textFormat = format;
}

const QString &BaseHoverHandler::toolTip() const
{
    return m_toolTip;
}

void BaseHoverHandler::setLastHelpItemIdentified(const Core::HelpItem &help)
{
    m_lastHelpItemIdentified = help;
}

const Core::HelpItem &BaseHoverHandler::lastHelpItemIdentified() const
{
    return m_lastHelpItemIdentified;
}

bool BaseHoverHandler::isContextHelpRequest() const
{
    return m_isContextHelpRequest;
}

void BaseHoverHandler::propagateHelpId(TextEditorWidget *widget,
                                       const Core::IContext::HelpCallback &callback)
{
    const Core::HelpItem contextHelp = lastHelpItemIdentified();
    widget->setContextHelpItem(contextHelp);
    callback(contextHelp);
}

void BaseHoverHandler::process(TextEditorWidget *widget, int pos, ReportPriority report)
{
    m_toolTip.clear();
    m_priority = -1;
    m_lastHelpItemIdentified = Core::HelpItem();

    identifyMatch(widget, pos, report);
}

void BaseHoverHandler::identifyMatch(TextEditorWidget *editorWidget, int pos, ReportPriority report)
{
    Utils::ExecuteOnDestruction reportPriority([this, report](){ report(priority()); });

    QString tooltip = editorWidget->extraSelectionTooltip(pos);
    if (!tooltip.isEmpty())
        setToolTip(tooltip);
}

void BaseHoverHandler::operateTooltip(TextEditorWidget *editorWidget, const QPoint &point)
{
    const QVariant helpItem = m_lastHelpItemIdentified.isEmpty()
                                  ? QVariant()
                                  : QVariant::fromValue(m_lastHelpItemIdentified);
    const bool extractHelp = m_lastHelpItemIdentified.isValid()
                             && !m_lastHelpItemIdentified.isFuzzyMatch();
    const QString helpContents = extractHelp ? m_lastHelpItemIdentified.firstParagraph()
                                             : QString();
    if (m_toolTip.isEmpty() && helpContents.isEmpty()) {
        Utils::ToolTip::hide();
    } else {
        if (helpContents.isEmpty()) {
            Utils::ToolTip::show(point, m_toolTip, m_textFormat, editorWidget, helpItem);
        } else if (m_toolTip.isEmpty()) {
            Utils::ToolTip::show(point, helpContents, Qt::RichText, editorWidget, helpItem);
        } else {
            // separate labels for tool tip text and help,
            // so the text format (plain, rich, markdown) can be handled differently
            auto layout = new QVBoxLayout;
            layout->setContentsMargins(0, 0, 0, 0);
            auto label = new QLabel;
            label->setObjectName("qcWidgetTipTopLabel");
            label->setTextFormat(m_textFormat);
            label->setText(m_toolTip);
            layout->addWidget(label);
            auto helpContentLabel = new QLabel("<hr/>" + helpContents);
            helpContentLabel->setObjectName("qcWidgetTipHelpLabel");
            layout->addWidget(helpContentLabel);
            Utils::ToolTip::show(point, layout, editorWidget, helpItem);
        }
    }
}

} // namespace TextEditor
