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

#include "clangfixitoperationsextractor.h"

#include "clangfixitoperation.h"
#include "clangutils.h"

#include <utils/algorithm.h>
#include <utils/link.h>

#include <QDebug>

namespace ClangCodeModel {
namespace Internal {

namespace {

void capitalizeText(QString &text)
{
    text[0] = text[0].toUpper();
}

QString tweakedDiagnosticText(const QString &diagnosticText)
{
    // Examples:
    // "note: use '==' to turn this assignment into an equality comparison"
    // "error: expected ';' at end of declaration"

    QString tweakedText = diagnosticText;

    if (!tweakedText.isEmpty()) {
        tweakedText = diagnosticCategoryPrefixRemoved(tweakedText);
        capitalizeText(tweakedText);
    }

    return tweakedText;
}

bool hasFixItAt(const QList<ClangFixIt> &fixits, const QString &filePath, int line)
{
    const auto isFixitForLocation = [filePath, line] (const ClangFixIt &fixit) {
        const Utils::Link &location = fixit.range.start;
        return location.targetFilePath.toString() == filePath && location.targetLine == line;
    };
    return Utils::anyOf(fixits, isFixitForLocation);
}

} // anonymous namespace

ClangFixItOperationsExtractor::ClangFixItOperationsExtractor(
        const QList<ClangDiagnostic> &diagnosticContainers)
    : diagnostics(diagnosticContainers)
{
}

TextEditor::QuickFixOperations
ClangFixItOperationsExtractor::extract(const QString &filePath, int line)
{
    foreach (const ClangDiagnostic &diagnostic, diagnostics)
        extractFromDiagnostic(diagnostic, filePath, line);

    return operations;
}

void ClangFixItOperationsExtractor::appendFixitOperation(
        const QString &diagnosticText,
        const QList<ClangFixIt> &fixits)
{
    if (!fixits.isEmpty()) {
        const QString diagnosticTextTweaked = tweakedDiagnosticText(diagnosticText);
        TextEditor::QuickFixOperation::Ptr operation(
                    new ClangFixItOperation(diagnosticTextTweaked, fixits));
        operations.append(operation);
    }
}

void ClangFixItOperationsExtractor::extractFromDiagnostic(
        const ClangDiagnostic &diagnostic,
        const QString &filePath,
        int line)
{
    const QList<ClangFixIt> &fixIts = diagnostic.fixIts;
    if (hasFixItAt(fixIts, filePath, line)) {
        appendFixitOperation(diagnostic.text, fixIts);
        for (const auto &child : diagnostic.children)
            extractFromDiagnostic(child, filePath, line);
    }
}

} // namespace Internal
} // namespace ClangCodeModel
