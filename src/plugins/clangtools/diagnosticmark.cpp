/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#include "diagnosticmark.h"

#include "clangtoolsutils.h"

#include <utils/utilsicons.h>

namespace ClangTools {
namespace Internal {

DiagnosticMark::DiagnosticMark(const Diagnostic &diagnostic)
    : TextEditor::TextMark(Utils::FilePath::fromString(diagnostic.location.filePath),
                           diagnostic.location.line,
                           Utils::Id("ClangTool.DiagnosticMark"))
    , m_diagnostic(diagnostic)
{
    if (diagnostic.type == "error" || diagnostic.type == "fatal")
        setColor(Utils::Theme::CodeModel_Error_TextMarkColor);
    else
        setColor(Utils::Theme::CodeModel_Warning_TextMarkColor);
    setPriority(TextEditor::TextMark::HighPriority);
    QIcon markIcon = diagnostic.icon();
    setIcon(markIcon.isNull() ? Utils::Icons::CODEMODEL_WARNING.icon() : markIcon);
    setToolTip(createDiagnosticToolTipString(diagnostic, Utils::nullopt,  true));
    setLineAnnotation(diagnostic.description);
}

void DiagnosticMark::disable()
{
    if (!m_enabled)
        return;
    m_enabled = false;
    if (m_diagnostic.type == "error" || m_diagnostic.type == "fatal")
        setIcon(Utils::Icons::CODEMODEL_DISABLED_ERROR.icon());
    else
        setIcon(Utils::Icons::CODEMODEL_DISABLED_WARNING.icon());
    setColor(Utils::Theme::Color::IconsDisabledColor);
    updateMarker();
}

bool DiagnosticMark::enabled() const
{
    return m_enabled;
}

} // namespace Internal
} // namespace ClangTools

