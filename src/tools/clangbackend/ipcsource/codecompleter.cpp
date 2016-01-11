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
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
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
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "codecompleter.h"

#include "clangcodecompleteresults.h"
#include "clangstring.h"
#include "cursor.h"
#include "codecompletefailedexception.h"
#include "codecompletionsextractor.h"
#include "sourcelocation.h"
#include "temporarymodifiedunsavedfiles.h"
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

    if (results.hasNoResultsForDotCompletion())
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
    const auto options = CXCodeComplete_IncludeMacros | CXCodeComplete_IncludeCodePatterns;

    return clang_codeCompleteAt(translationUnit.cxTranslationUnitWithoutReparsing(),
                                translationUnit.filePath().constData(),
                                line,
                                column,
                                unsavedFiles,
                                unsavedFileCount,
                                options);
}

ClangCodeCompleteResults CodeCompleter::completeWithArrowInsteadOfDot(uint line, uint column)
{
    TemporaryModifiedUnsavedFiles modifiedUnsavedFiles(translationUnit.cxUnsavedFilesVector());
    const SourceLocation location = translationUnit.sourceLocationAtWithoutReparsing(line,
                                                                                     column - 1);

    const bool replaced = modifiedUnsavedFiles.replaceInFile(filePath(),
                                                             location.offset(),
                                                             1,
                                                             Utf8StringLiteral("->"));

    ClangCodeCompleteResults results;

    if (replaced) {
        results = complete(line,
                           column + 1,
                           modifiedUnsavedFiles.cxUnsavedFiles(),
                           modifiedUnsavedFiles.count());

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

