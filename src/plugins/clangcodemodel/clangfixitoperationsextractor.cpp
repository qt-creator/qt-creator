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

#include "clangfixitoperationsextractor.h"

#include "clangfixitoperation.h"

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

bool isDiagnosticFromFileAtLine(const DiagnosticContainer &diagnosticContainer,
                                const QString &filePath,
                                int line)
{
    const auto location = diagnosticContainer.location();
    return location.filePath().toString() == filePath
        && location.line() == uint(line);
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

void ClangFixItOperationsExtractor::appendFixitOperationsFromDiagnostic(
        const QString &filePath,
        const DiagnosticContainer &diagnosticContainer)
{
    const auto fixIts = diagnosticContainer.fixIts();
    if (!fixIts.isEmpty()) {
        const QString diagnosticText = tweakedDiagnosticText(diagnosticContainer.text().toString());
        TextEditor::QuickFixOperation::Ptr operation(
                    new ClangFixItOperation(filePath,
                                            diagnosticText,
                                            fixIts));
        operations.append(operation);
    }
}

void ClangFixItOperationsExtractor::extractFromDiagnostic(
        const DiagnosticContainer &diagnosticContainer,
        const QString &filePath,
        int line)
{
    if (isDiagnosticFromFileAtLine(diagnosticContainer, filePath, line)) {
        appendFixitOperationsFromDiagnostic(filePath, diagnosticContainer);

        foreach (const auto &child, diagnosticContainer.children())
            extractFromDiagnostic(child, filePath, line);
    }
}

} // namespace ClangCodeModel
