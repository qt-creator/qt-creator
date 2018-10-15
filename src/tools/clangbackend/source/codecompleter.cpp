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

#include "codecompleter.h"

#include "clangbackend_global.h"
#include "clangcodecompleteresults.h"
#include "clangdocument.h"
#include "clangexceptions.h"
#include "clangfilepath.h"
#include "clangstring.h"
#include "clangtranslationunitupdater.h"
#include "clangunsavedfilesshallowarguments.h"
#include "codecompletionsextractor.h"
#include "cursor.h"
#include "sourcelocation.h"
#include "sourcerange.h"
#include "unsavedfile.h"
#include "unsavedfiles.h"

#include <utils/qtcassert.h>

#include <clang-c/Index.h>

namespace ClangBackEnd {

namespace {

CodeCompletions toCodeCompletions(const UnsavedFile &unsavedFile,
                                  const ClangCodeCompleteResults &results,
                                  bool onlyFunctionOverloads)
{
    if (results.isNull())
        return CodeCompletions();

    CodeCompletionsExtractor extractor(unsavedFile, results.data());
    CodeCompletions codeCompletions = extractor.extractAll(onlyFunctionOverloads);

    return codeCompletions;
}

void filterUnknownContextResults(ClangCodeCompleteResults &results,
                                 const UnsavedFile &theUnsavedFile,
                                 uint line,
                                 uint column)
{
    if (results.hasUnknownContext()) {
        bool positionIsOk = false;
        const uint position = theUnsavedFile.toUtf8Position(line, column - 1, &positionIsOk);
        if (positionIsOk && (theUnsavedFile.hasCharacterAt(position, '.')
                             || (theUnsavedFile.hasCharacterAt(position - 1, '-')
                                 && theUnsavedFile.hasCharacterAt(position, '>')))) {
            results = {};
        }
    }
}

} // anonymous namespace

CodeCompleter::CodeCompleter(const TranslationUnit &translationUnit,
                             const UnsavedFiles &unsavedFiles)
    : translationUnit(translationUnit)
    , unsavedFiles(unsavedFiles)
{
}

static void replaceWithOpeningParen(UnsavedFile &file, uint line, uint column)
{
    bool ok;
    const uint pos = file.toUtf8Position(line, column, &ok);
    QTC_ASSERT(ok, return;);
    file.replaceAt(pos, 1, Utf8String("(", 1));
}

CodeCompletions CodeCompleter::complete(uint line, uint column,
                                        int funcNameStartLine,
                                        int funcNameStartColumn)
{
    if (funcNameStartLine >= 0) {
        UnsavedFile &file = unsavedFiles.unsavedFile(translationUnit.filePath());
        // Replace '{' by '(' to get proper FunctionOverloadCompletionKind for constructor.
        if (file.hasCharacterAt(line, column - 1, '{'))
            replaceWithOpeningParen(file, line, column - 1);
    }

    // Check if we have a smart pointer completion and get proper constructor signatures in results.
    // Results are empty when it's not a smart pointer or this completion failed.
    ClangCodeCompleteResults clangCompletions = completeSmartPointerCreation(line,
                                                                             column,
                                                                             funcNameStartLine,
                                                                             funcNameStartColumn);

    // Default completion.
    if (clangCompletions.isNull() || clangCompletions.isEmpty())
        clangCompletions = completeHelper(line, column);

    filterUnknownContextResults(clangCompletions, unsavedFile(), line, column);

    return toCodeCompletions(unsavedFiles.unsavedFile(translationUnit.filePath()),
                             clangCompletions,
                             funcNameStartLine >= 0);
}

// For given "make_unique<T>" / "make_shared<T>" / "QSharedPointer<T>::create" return "new T("
// Otherwize return empty QString
static QString tweakName(const Utf8String &oldName)
{
    if (!oldName.contains('>'))
        return QString();

    QString fullName = QString(oldName).trimmed();
    if (!fullName.endsWith('>')) {
        // This is the class<type>::method case - remove ::method part
        if (!fullName.endsWith("create") || !fullName.contains("QSharedPointer"))
            return QString();
        fullName = fullName.mid(0, fullName.lastIndexOf(':') - 1);
    } else if (!fullName.contains("make_unique") && !fullName.contains("make_shared")) {
        return QString();
    }
    int templateStart = fullName.indexOf('<');
    QString templatePart = fullName.mid(templateStart + 1,
                                        fullName.length() - templateStart - 2);
    return "new " + templatePart + "(";
}

ClangCodeCompleteResults CodeCompleter::completeSmartPointerCreation(uint line,
                                                                     uint column,
                                                                     int funcNameStartLine,
                                                                     int funcNameStartColumn)
{
    if (column <= 1 || funcNameStartLine == -1)
        return ClangCodeCompleteResults();

    UnsavedFile &file = unsavedFiles.unsavedFile(translationUnit.filePath());
    if (!file.hasCharacterAt(line, column - 1, '('))
        return ClangCodeCompleteResults();

    bool ok;
    const uint startPos = file.toUtf8Position(funcNameStartLine, funcNameStartColumn, &ok);
    QTC_ASSERT(ok, return ClangCodeCompleteResults(););
    const uint endPos = file.toUtf8Position(line, column - 1, &ok);
    QTC_ASSERT(ok, return ClangCodeCompleteResults(););

    const Utf8String content = file.fileContent();
    const Utf8String oldName = content.mid(startPos, endPos - startPos);
    const QString updatedName = tweakName(oldName);
    if (updatedName.isEmpty())
        return ClangCodeCompleteResults();

    column += updatedName.length();
    file.replaceAt(endPos + 1, 0, updatedName);

    ClangCodeCompleteResults results = completeHelper(line, column);
    if (results.isEmpty()) {
        column -= updatedName.length();
        file.replaceAt(endPos + 1, updatedName.length(), QString());
    }
    return results;
}

ClangCodeCompleteResults CodeCompleter::completeHelper(uint line, uint column)
{
    const Utf8String nativeFilePath = FilePath::toNativeSeparators(translationUnit.filePath());
    UnsavedFilesShallowArguments unsaved = unsavedFiles.shallowArguments();

    return clang_codeCompleteAt(translationUnit.cxTranslationUnit(),
                                nativeFilePath.constData(),
                                line,
                                column,
                                unsaved.data(),
                                unsaved.count(),
                                defaultOptions());
}

uint CodeCompleter::defaultOptions() const
{
    uint options = CXCodeComplete_IncludeMacros
        #ifdef IS_COMPLETION_FIXITS_BACKPORTED
            | CXCodeComplete_IncludeCompletionsWithFixIts
        #endif
            | CXCodeComplete_IncludeCodePatterns;

    if (TranslationUnitUpdater::defaultParseOptions()
            & CXTranslationUnit_IncludeBriefCommentsInCodeCompletion) {
        options |= CXCodeComplete_IncludeBriefComments;
    }

    return options;
}

UnsavedFile &CodeCompleter::unsavedFile()
{
    return unsavedFiles.unsavedFile(translationUnit.filePath());
}

} // namespace ClangBackEnd

