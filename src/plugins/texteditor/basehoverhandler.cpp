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

#include "basehoverhandler.h"
#include "texteditor.h"

#include <coreplugin/icore.h>
#include <utils/tooltip/tooltip.h>

#include <QPoint>

using namespace Core;

namespace TextEditor {

BaseHoverHandler::BaseHoverHandler() : m_diagnosticTooltip(false)
{
}

BaseHoverHandler::~BaseHoverHandler()
{}

void BaseHoverHandler::showToolTip(TextEditorWidget *widget, const QPoint &point, int pos)
{
    widget->setContextHelpId(QString());

    process(widget, pos);
    operateTooltip(widget, point);
}

QString BaseHoverHandler::contextHelpId(TextEditorWidget *widget, int pos)
{
    // If the tooltip is visible and there is a help match, this match is used to update
    // the help id. Otherwise, let the identification process happen.
    if (!Utils::ToolTip::isVisible() || !lastHelpItemIdentified().isValid())
        process(widget, pos);

    if (lastHelpItemIdentified().isValid())
        return lastHelpItemIdentified().helpId();
    return QString();
}

void BaseHoverHandler::setToolTip(const QString &tooltip)
{
    m_toolTip = tooltip;
}

const QString &BaseHoverHandler::toolTip() const
{
    return m_toolTip;
}

void BaseHoverHandler::appendToolTip(const QString &extension)
{
    m_toolTip.append(extension);
}

void BaseHoverHandler::addF1ToToolTip()
{
    m_toolTip = QString::fromLatin1("<table><tr><td valign=middle>%1</td><td>&nbsp;&nbsp;"
                                    "<img src=\":/texteditor/images/f1.png\"></td>"
                                    "</tr></table>").arg(m_toolTip);
}

void BaseHoverHandler::setIsDiagnosticTooltip(bool isDiagnosticTooltip)
{
    m_diagnosticTooltip = isDiagnosticTooltip;
}

bool BaseHoverHandler::isDiagnosticTooltip() const
{
    return m_diagnosticTooltip;
}

void BaseHoverHandler::setLastHelpItemIdentified(const HelpItem &help)
{
    m_lastHelpItemIdentified = help;
}

const HelpItem &BaseHoverHandler::lastHelpItemIdentified() const
{
    return m_lastHelpItemIdentified;
}

void BaseHoverHandler::clear()
{
    m_diagnosticTooltip = false;
    m_toolTip.clear();
    m_lastHelpItemIdentified = HelpItem();
}

void BaseHoverHandler::process(TextEditorWidget *widget, int pos)
{
    clear();
    identifyMatch(widget, pos);
    decorateToolTip();
}

void BaseHoverHandler::decorateToolTip()
{
    if (Qt::mightBeRichText(toolTip()))
        setToolTip(toolTip().toHtmlEscaped());

    if (!isDiagnosticTooltip() && lastHelpItemIdentified().isValid()) {
        const QString &contents = lastHelpItemIdentified().extractContent(false);
        if (!contents.isEmpty()) {
            setToolTip(toolTip().toHtmlEscaped());
            appendToolTip(contents);
            addF1ToToolTip();
        }
    }
}

void BaseHoverHandler::operateTooltip(TextEditorWidget *editorWidget, const QPoint &point)
{
    if (m_toolTip.isEmpty())
        Utils::ToolTip::hide();
    else
        Utils::ToolTip::show(point, m_toolTip, editorWidget);
}

} // namespace TextEditor
