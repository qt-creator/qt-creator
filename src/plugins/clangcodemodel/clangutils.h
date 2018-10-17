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

#pragma once

#include <cplusplus/Icons.h>

#include <cpptools/projectpart.h>

#include <QTextCursor>

QT_BEGIN_NAMESPACE
class QTextBlock;
QT_END_NAMESPACE

namespace CppTools {
class CppEditorDocumentHandle;
class ProjectInfo;
}

namespace Utils {
class FileName;
}

namespace ClangBackEnd { class TokenInfoContainer; }

namespace ClangCodeModel {
namespace Utils {

CppTools::CppEditorDocumentHandle *cppDocument(const QString &filePath);
void setLastSentDocumentRevision(const QString &filePath, uint revision);

QStringList createClangOptions(const CppTools::ProjectPart &projectPart,
                               CppTools::ProjectFile::Kind fileKind);

CppTools::ProjectPart::Ptr projectPartForFile(const QString &filePath);
CppTools::ProjectPart::Ptr projectPartForFileBasedOnProcessor(const QString &filePath);
bool isProjectPartLoaded(const CppTools::ProjectPart::Ptr projectPart);
QString projectPartIdForFile(const QString &filePath);
int clangColumn(const QTextBlock &line, int cppEditorColumn);
int cppEditorColumn(const QTextBlock &line, int clangColumn);

QString diagnosticCategoryPrefixRemoved(const QString &text);

::Utils::CodeModelIcon::Type iconTypeForToken(const ClangBackEnd::TokenInfoContainer &token);

void generateCompilationDB(::Utils::FileName projectDir, CppTools::ProjectInfo projectInfo);

namespace Text {

template <class CharacterProvider>
void moveToPreviousChar(CharacterProvider &provider, QTextCursor &cursor)
{
    cursor.movePosition(QTextCursor::PreviousCharacter);
    while (provider.characterAt(cursor.position()).isSpace())
        cursor.movePosition(QTextCursor::PreviousCharacter);
}

template <class CharacterProvider>
void moveToPreviousWord(CharacterProvider &provider, QTextCursor &cursor)
{
    cursor.movePosition(QTextCursor::PreviousWord);
    while (provider.characterAt(cursor.position()) == ':')
        cursor.movePosition(QTextCursor::PreviousWord, QTextCursor::MoveAnchor, 2);
}

template <class CharacterProvider>
bool matchPreviousWord(CharacterProvider &provider, QTextCursor cursor, QString pattern)
{
    cursor.movePosition(QTextCursor::PreviousWord);
    while (provider.characterAt(cursor.position()) == ':')
        cursor.movePosition(QTextCursor::PreviousWord, QTextCursor::MoveAnchor, 2);

    int previousWordStart = cursor.position();
    cursor.movePosition(QTextCursor::NextWord);
    moveToPreviousChar(provider, cursor);
    QString toMatch = provider.textAt(previousWordStart, cursor.position() - previousWordStart + 1);

    pattern = pattern.simplified();
    while (!pattern.isEmpty() && pattern.endsWith(toMatch)) {
        pattern.chop(toMatch.length());
        if (pattern.endsWith(' '))
            pattern.chop(1);
        if (!pattern.isEmpty()) {
            cursor.movePosition(QTextCursor::StartOfWord);
            cursor.movePosition(QTextCursor::PreviousWord);
            previousWordStart = cursor.position();
            cursor.movePosition(QTextCursor::NextWord);
            moveToPreviousChar(provider, cursor);
            toMatch = provider.textAt(previousWordStart, cursor.position() - previousWordStart + 1);
        }
    }
    return pattern.isEmpty();
}

} // namespace Text
} // namespace Utils
} // namespace Clang
