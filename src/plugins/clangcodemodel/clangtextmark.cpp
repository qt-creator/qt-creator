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

#include "clangtextmark.h"

#include "clangconstants.h"

#include <utils/icon.h>
#include <utils/theme/theme.h>

#include <QString>

namespace ClangCodeModel {

namespace {

bool isWarningOrNote(ClangBackEnd::DiagnosticSeverity severity)
{
    using ClangBackEnd::DiagnosticSeverity;
    switch (severity) {
        case DiagnosticSeverity::Ignored:
        case DiagnosticSeverity::Note:
        case DiagnosticSeverity::Warning: return true;
        case DiagnosticSeverity::Error:
        case DiagnosticSeverity::Fatal: return false;
    }

    Q_UNREACHABLE();
}

Core::Id cartegoryForSeverity(ClangBackEnd::DiagnosticSeverity severity)
{
    return isWarningOrNote(severity) ? Constants::CLANG_WARNING : Constants::CLANG_ERROR;
}

} // anonymous namespace

ClangTextMark::ClangTextMark(const QString &fileName, int lineNumber, ClangBackEnd::DiagnosticSeverity severity)
    : TextEditor::TextMark(fileName, lineNumber, cartegoryForSeverity(severity))
{
    setPriority(TextEditor::TextMark::HighPriority);
    setIcon(severity);
}

void ClangTextMark::setIcon(ClangBackEnd::DiagnosticSeverity severity)
{
    static const QIcon errorIcon = Utils::Icon({
            {QLatin1String(":/clangcodemodel/images/error.png"), Utils::Theme::IconsErrorColor}
        }, Utils::Icon::Tint).icon();
    static const QIcon warningIcon = Utils::Icon({
            {QLatin1String(":/clangcodemodel/images/warning.png"), Utils::Theme::IconsWarningColor}
        }, Utils::Icon::Tint).icon();

    if (isWarningOrNote(severity))
        TextMark::setIcon(warningIcon);
    else
        TextMark::setIcon(errorIcon);
}

ClangTextMark::ClangTextMark(ClangTextMark &&other) Q_DECL_NOEXCEPT
    : TextMark(std::move(other))
{
}

} // namespace ClangCodeModel

