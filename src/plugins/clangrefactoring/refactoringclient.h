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

#include "refactoringengine.h"

#include <searchhandle.h>

#include <refactoringclientinterface.h>

#include <functional>

namespace ClangBackEnd {
class FilePath;
class RefactoringConnectionClient;
class SourceRangesContainer;
class SourceRangeWithTextContainer;
}

namespace ClangRefactoring {

class ClangQueryExampleHighlighter;
class ClangQueryHighlighter;

class RefactoringClient final : public ClangBackEnd::RefactoringClientInterface
{
public:
    void alive() override;
    void sourceLocationsForRenamingMessage(
            ClangBackEnd::SourceLocationsForRenamingMessage &&message) override;
    void sourceRangesAndDiagnosticsForQueryMessage(
            ClangBackEnd::SourceRangesAndDiagnosticsForQueryMessage &&message) override;
    void sourceRangesForQueryMessage(
            ClangBackEnd::SourceRangesForQueryMessage &&message) override;

    void setLocalRenamingCallback(
            CppTools::RefactoringEngineInterface::RenameCallback &&localRenamingCallback) override;
    void setRefactoringEngine(ClangRefactoring::RefactoringEngine *refactoringEngine);
    void setSearchHandle(ClangRefactoring::SearchHandle *searchHandleInterface);
    ClangRefactoring::SearchHandle *searchHandle() const;
    void setClangQueryExampleHighlighter(ClangQueryExampleHighlighter *highlighter);
    void setClangQueryHighlighter(ClangQueryHighlighter *highlighter);

    bool hasValidLocalRenamingCallback() const;

    void setExpectedResultCount(uint count);
    uint expectedResultCount() const;
    uint resultCounter() const;

    void setRefactoringConnectionClient(ClangBackEnd::RefactoringConnectionClient *connectionClient);

unittest_public:
    void addSearchResult(const ClangBackEnd::SourceRangeWithTextContainer &sourceRange);

private:
    void addSearchResults(const ClangBackEnd::SourceRangesContainer &sourceRanges);

    void setResultCounterAndSendSearchIsFinishedIfFinished();
    void sendSearchIsFinished();

private:
    CppTools::RefactoringEngineInterface::RenameCallback m_localRenamingCallback;
    ClangBackEnd::RefactoringConnectionClient *m_connectionClient = nullptr;
    SearchHandle *m_searchHandle = nullptr;
    RefactoringEngine *m_refactoringEngine = nullptr;
    ClangQueryExampleHighlighter *m_clangQueryExampleHighlighter = nullptr;
    ClangQueryHighlighter *m_clangQueryHighlighter = nullptr;
    uint m_expectedResultCount = 0;
    uint m_resultCounter = 0;
};

} // namespace ClangRefactoring
