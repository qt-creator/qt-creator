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

#include "clangfilepath.h"
#include "clangcodecompleteresults.h"
#include "clangstring.h"
#include "cursor.h"
#include "clangexceptions.h"
#include "codecompletionsextractor.h"
#include "sourcelocation.h"
#include "unsavedfile.h"
#include "unsavedfiles.h"
#include "clangdocument.h"
#include "sourcerange.h"
#include "clangunsavedfilesshallowarguments.h"
#include "clangtranslationunitupdater.h"

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

CodeCompleter::CodeCompleter(const TranslationUnit &translationUnit,
                             const UnsavedFiles &unsavedFiles)
    : translationUnit(translationUnit)
    , unsavedFiles(unsavedFiles)
{
}

CodeCompletions CodeCompleter::complete(uint line, uint column)
{
    neededCorrection_ = CompletionCorrection::NoCorrection;

    ClangCodeCompleteResults results = completeHelper(line, column);

    tryDotArrowCorrectionIfNoResults(results, line, column);

    return toCodeCompletions(results);
}

CompletionCorrection CodeCompleter::neededCorrection() const
{
    return neededCorrection_;
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

void CodeCompleter::tryDotArrowCorrectionIfNoResults(ClangCodeCompleteResults &results,
                                                     uint line,
                                                     uint column)
{
    if (results.hasNoResultsForDotCompletion()) {
        const UnsavedFile &theUnsavedFile = unsavedFile();
        bool positionIsOk = false;
        const uint dotPosition = theUnsavedFile.toUtf8Position(line, column - 1, &positionIsOk);
        if (positionIsOk && theUnsavedFile.hasCharacterAt(dotPosition, '.'))
            results = completeWithArrowInsteadOfDot(line, column, dotPosition);
    }
}

ClangCodeCompleteResults CodeCompleter::completeWithArrowInsteadOfDot(uint line,
                                                                      uint column,
                                                                      uint dotPosition)
{
    ClangCodeCompleteResults results;
    const bool replaced = unsavedFile().replaceAt(dotPosition,
                                                  1,
                                                  Utf8StringLiteral("->"));

    if (replaced) {
        results = completeHelper(line, column + 1);
        if (results.hasResults())
            neededCorrection_ = CompletionCorrection::DotToArrowCorrection;
    }

    return results;
}

} // namespace ClangBackEnd

