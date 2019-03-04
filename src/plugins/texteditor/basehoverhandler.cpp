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

namespace TextEditor {

BaseHoverHandler::~BaseHoverHandler() = default;

void BaseHoverHandler::showToolTip(TextEditorWidget *widget, const QPoint &point, bool decorate)
{
    if (decorate)
        decorateToolTip();
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

void BaseHoverHandler::setToolTip(const QString &tooltip)
{
    m_toolTip = tooltip;
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

void BaseHoverHandler::decorateToolTip()
{
    if (Qt::mightBeRichText(toolTip()))
        setToolTip(toolTip().toHtmlEscaped());

    if (lastHelpItemIdentified().isValid() && !lastHelpItemIdentified().isFuzzyMatch()) {
        const QString &helpContents = lastHelpItemIdentified().extractContent(false);
        if (!helpContents.isEmpty()) {
            m_toolTip = toolTip().toHtmlEscaped();
            m_toolTip = m_toolTip.isEmpty() ? helpContents : ("<p>" + m_toolTip + "</p><hr/><p>" + helpContents + "</p>");
        }
    }
}

void BaseHoverHandler::operateTooltip(TextEditorWidget *editorWidget, const QPoint &point)
{
    if (m_toolTip.isEmpty())
        Utils::ToolTip::hide();
    else
        Utils::ToolTip::show(point,
                             m_toolTip,
                             editorWidget,
                             qVariantFromValue(m_lastHelpItemIdentified));
}

} // namespace TextEditor
