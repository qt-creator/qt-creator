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

#include <utils/algorithm.h>

#include <QDebug>

using ClangBackEnd::DiagnosticContainer;

namespace {

void capitalizeText(QString &text)
{
    text[0] = text[0].toUpper();
}

void removeDiagnosticCategoryPrefix(QString &text)
{
    // Prefixes are taken from $LLVM_SOURCE_DIR/tools/clang/lib/Frontend/TextDiagnostic.cpp,
    // function TextDiagnostic::printDiagnosticLevel (llvm-3.6.2).
    static const QStringList categoryPrefixes = {
        QStringLiteral("note"),
        QStringLiteral("remark"),
        QStringLiteral("warning"),
        QStringLiteral("error"),
        QStringLiteral("fatal error")
    };

    foreach (const QString &prefix, categoryPrefixes) {
        const QString fullPrefix = prefix + QStringLiteral(": ");
        if (text.startsWith(fullPrefix)) {
            text.remove(0, fullPrefix.length());
            break;
        }
    }
}

QString tweakedDiagnosticText(const QString &diagnosticText)
{
    // Examples:
    // "note: use '==' to turn this assignment into an equality comparison"
    // "error: expected ';' at end of declaration"

    QString tweakedText = diagnosticText;

    if (!tweakedText.isEmpty()) {
        removeDiagnosticCategoryPrefix(tweakedText);
        capitalizeText(tweakedText);
    }

    return tweakedText;
}

bool hasFixItAt(const QVector<ClangBackEnd::FixItContainer> &fixits,
                const Utf8String &filePath,
                int line)
{
    const auto isFixitForLocation = [filePath, line] (const ClangBackEnd::FixItContainer &fixit) {
        const ClangBackEnd::SourceLocationContainer location = fixit.range().start();
        return location.filePath() == filePath && location.line() == uint(line);
    };

    return Utils::anyOf(fixits, isFixitForLocation);
}

} // anonymous namespace

namespace ClangCodeModel {

ClangFixItOperationsExtractor::ClangFixItOperationsExtractor(
        const QVector<DiagnosticContainer> &diagnosticContainers)
    : diagnosticContainers(diagnosticContainers)
{
}

TextEditor::QuickFixOperations
ClangFixItOperationsExtractor::extract(const QString &filePath, int line)
{
    foreach (const DiagnosticContainer &diagnosticContainer, diagnosticContainers)
        extractFromDiagnostic(diagnosticContainer, filePath, line);

    return operations;
}

void ClangFixItOperationsExtractor::appendFixitOperation(
        const QString &diagnosticText,
        const QVector<ClangBackEnd::FixItContainer> &fixits)
{
    if (!fixits.isEmpty()) {
        const QString diagnosticTextTweaked = tweakedDiagnosticText(diagnosticText);
        TextEditor::QuickFixOperation::Ptr operation(
                    new ClangFixItOperation(diagnosticTextTweaked, fixits));
        operations.append(operation);
    }
}

void ClangFixItOperationsExtractor::extractFromDiagnostic(
        const DiagnosticContainer &diagnosticContainer,
        const QString &filePath,
        int line)
{
    const QVector<ClangBackEnd::FixItContainer> fixIts = diagnosticContainer.fixIts();
    if (hasFixItAt(fixIts, filePath, line)) {
        appendFixitOperation(diagnosticContainer.text().toString(), fixIts);

        foreach (const auto &child, diagnosticContainer.children())
            extractFromDiagnostic(child, filePath, line);
    }
}

} // namespace ClangCodeModel
