/****************************************************************************
**
** Copyright (C) 2018 Sergey Morozov
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

#include "cppcheckconstants.h"
#include "cppcheckdiagnostic.h"
#include "cppchecktextmark.h"

#include <utils/utilsicons.h>

#include <QMap>

namespace Cppcheck {
namespace Internal {

struct Visual
{
    Visual(Utils::Theme::Color color, TextEditor::TextMark::Priority priority,
           const QIcon &icon)
        : color(color),
          priority(priority),
          icon(icon)
    {}
    Utils::Theme::Color color;
    TextEditor::TextMark::Priority priority;
    QIcon icon;
};

static Visual getVisual(Diagnostic::Severity type)
{
    using Color = Utils::Theme::Color;
    using Priority = TextEditor::TextMark::Priority;

    static const QMap<Diagnostic::Severity, Visual> visuals{
        {Diagnostic::Severity::Error, {Color::IconsErrorColor, Priority::HighPriority,
                        Utils::Icons::CRITICAL.icon()}},
        {Diagnostic::Severity::Warning, {Color::IconsWarningColor, Priority::NormalPriority,
                        Utils::Icons::WARNING.icon()}},
    };

    return visuals.value(type, {Color::IconsInfoColor, Priority::LowPriority,
                                Utils::Icons::INFO.icon()});
}

CppcheckTextMark::CppcheckTextMark (const Diagnostic &diagnostic)
    : TextEditor::TextMark (diagnostic.fileName, diagnostic.lineNumber,
                            Core::Id(Constants::TEXTMARK_CATEGORY_ID)),
    m_severity(diagnostic.severity),
    m_checkId(diagnostic.checkId),
    m_message(diagnostic.message)
{
    const Visual visual = getVisual(diagnostic.severity);
    setPriority(visual.priority);
    setColor(visual.color);
    setIcon(visual.icon);
    setToolTip(toolTipText(diagnostic.severityText));
    setLineAnnotation(diagnostic.message);
}

QString CppcheckTextMark::toolTipText(const QString &severityText) const
{
    return QString(
                "<table cellspacing='0' cellpadding='0' width='100%'>"
                "  <tr>"
                "    <td align='left'><b>Cppcheck</b></td>"
                "    <td align='right'>&nbsp;<font color='gray'>%1: %2</font></td>"
                "  </tr>"
                "  <tr>"
                "    <td colspan='2' align='left' style='padding-left:10px'>%3</td>"
                "  </tr>"
                "</table>").arg(m_checkId, severityText, m_message);
}

} // namespace Internal
} // namespace Cppcheck
