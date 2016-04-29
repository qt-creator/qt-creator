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

#include "clangcodecompleteresults.h"
#include "clangstring.h"
#include "cursor.h"
#include "codecompletefailedexception.h"
#include "codecompletionsextractor.h"
#include "sourcelocation.h"
#include "unsavedfile.h"
#include "clangtranslationunit.h"
#include "sourcerange.h"

#include <clang-c/Index.h>

namespace ClangBackEnd {

namespace {

CodeCompletions toCodeCompletions(const ClangCodeCompleteResults &results)
{
    if (results.isNull())
        return CodeCompletions();

    CodeCompletionsExtractor extractor(results.data());
    CodeCompletions codeCompletions = extractor.extractAll();

    return codeCompletions;
}

} // anonymous namespace

CodeCompleter::CodeCompleter(TranslationUnit translationUnit)
    : translationUnit(std::move(translationUnit))
{
}

CodeCompletions CodeCompleter::complete(uint line, uint column)
{
    neededCorrection_ = CompletionCorrection::NoCorrection;

    ClangCodeCompleteResults results = complete(line,
                                                column,
                                                translationUnit.cxUnsavedFiles(),
                                                translationUnit.unsavedFilesCount());

    if (results.hasNoResultsForDotCompletion() && hasDotAt(line, column - 1))
        results = completeWithArrowInsteadOfDot(line, column);

    return toCodeCompletions(results);
}

CompletionCorrection CodeCompleter::neededCorrection() const
{
    return neededCorrection_;
}

ClangCodeCompleteResults CodeCompleter::complete(uint line,
                                                 uint column,
                                                 CXUnsavedFile *unsavedFiles,
                                                 unsigned unsavedFileCount)
{
    return clang_codeCompleteAt(translationUnit.cxTranslationUnitWithoutReparsing(),
                                translationUnit.filePath().constData(),
                                line,
                                column,
                                unsavedFiles,
                                unsavedFileCount,
                                defaultOptions());
}

bool CodeCompleter::hasDotAt(uint line, uint column) const
{
    const UnsavedFile &unsavedFile = translationUnit.unsavedFile();

    return unsavedFile.hasCharacterAt(line, column, '.');
}

uint CodeCompleter::defaultOptions() const
{
    uint options = CXCodeComplete_IncludeMacros
                 | CXCodeComplete_IncludeCodePatterns;

    if (translationUnit.defaultOptions() & CXTranslationUnit_IncludeBriefCommentsInCodeCompletion)
        options |= CXCodeComplete_IncludeBriefComments;

    return options;
}

ClangCodeCompleteResults CodeCompleter::completeWithArrowInsteadOfDot(uint line, uint column)
{
    ClangCodeCompleteResults results;

    const SourceLocation location = translationUnit.sourceLocationAtWithoutReparsing(line, column - 1);
    const bool replaced = translationUnit.unsavedFile().replaceAt(location.offset(),
                                                                  1,
                                                                  Utf8StringLiteral("->"));

    if (replaced) {
        results = complete(line,
                           column + 1,
                           translationUnit.cxUnsavedFiles(),
                           translationUnit.unsavedFilesCount());

        if (results.hasResults())
            neededCorrection_ = CompletionCorrection::DotToArrowCorrection;
    }

    return results;
}

Utf8String CodeCompleter::filePath() const
{
    return translationUnit.filePath();
}

void CodeCompleter::checkCodeCompleteResult(CXCodeCompleteResults *completeResults)
{
    if (!completeResults)
        throw CodeCompleteFailedException();
}

} // namespace ClangBackEnd

