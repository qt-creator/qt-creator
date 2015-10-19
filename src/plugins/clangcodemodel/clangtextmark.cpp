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

#include "clangtextmark.h"

#include "constants.h"

#include <coreplugin/coreconstants.h>

#include <QString>
#include <QApplication>

#include <utils/tooltip/tooltip.h>
#include <utils/themehelper.h>

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
    static const QIcon errorIcon{Utils::ThemeHelper::themedIcon(QLatin1String(Core::Constants::ICON_ERROR))};
    static const QIcon warningIcon{Utils::ThemeHelper::themedIcon(QLatin1String(Core::Constants::ICON_WARNING))};

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

