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

#include "clanghighlightingmarksreporter.h"

#include <cpptools/semantichighlighter.h>

#include <QFuture>

namespace {

CppTools::SemanticHighlighter::Kind toCppToolsSemanticHighlighterKind(
        ClangBackEnd::HighlightingType type)
{
    using ClangBackEnd::HighlightingType;
    using CppTools::SemanticHighlighter;

    switch (type) {
    case HighlightingType::Keyword:
        return SemanticHighlighter::PseudoKeywordUse;
    case HighlightingType::Function:
        return SemanticHighlighter::FunctionUse;
    case HighlightingType::VirtualFunction:
        return SemanticHighlighter::VirtualMethodUse;
    case HighlightingType::Type:
        return SemanticHighlighter::TypeUse;
    case HighlightingType::LocalVariable:
        return SemanticHighlighter::LocalUse;
    case HighlightingType::Field:
        return SemanticHighlighter::FieldUse;
    case HighlightingType::GlobalVariable:
        return SemanticHighlighter::Unknown;
    case HighlightingType::Enumeration:
        return SemanticHighlighter::EnumerationUse;
    case HighlightingType::Label:
        return SemanticHighlighter::LabelUse;
    case HighlightingType::Preprocessor:
    case HighlightingType::PreprocessorDefinition:
    case HighlightingType::PreprocessorExpansion:
        return SemanticHighlighter::MacroUse;
    default:
        return SemanticHighlighter::Unknown;
    }

    Q_UNREACHABLE();
}

TextEditor::HighlightingResult toHighlightingResult(
        const ClangBackEnd::HighlightingMarkContainer &highlightingMark)
{
    const auto highlighterKind = toCppToolsSemanticHighlighterKind(highlightingMark.type());

    return TextEditor::HighlightingResult(highlightingMark.line(),
                                          highlightingMark.column(),
                                          highlightingMark.length(),
                                          highlighterKind);
}

} // anonymous

namespace ClangCodeModel {

HighlightingMarksReporter::HighlightingMarksReporter(
        const QVector<ClangBackEnd::HighlightingMarkContainer> &highlightingMarks)
    : m_highlightingMarks(highlightingMarks)
{
    m_chunksToReport.reserve(m_chunkSize + 1);
}

void HighlightingMarksReporter::reportChunkWise(
        const TextEditor::HighlightingResult &highlightingResult)
{
    if (m_chunksToReport.size() >= m_chunkSize) {
        if (m_flushRequested && highlightingResult.line != m_flushLine) {
            reportAndClearCurrentChunks();
        } else if (!m_flushRequested) {
            m_flushRequested = true;
            m_flushLine = highlightingResult.line;
        }
    }

    m_chunksToReport.append(highlightingResult);
}

void HighlightingMarksReporter::reportAndClearCurrentChunks()
{
    m_flushRequested = false;
    m_flushLine = 0;

    if (!m_chunksToReport.isEmpty()) {
        reportResults(m_chunksToReport);
        m_chunksToReport.erase(m_chunksToReport.begin(), m_chunksToReport.end());
    }
}

void HighlightingMarksReporter::setChunkSize(int chunkSize)
{
    m_chunkSize = chunkSize;
}

void HighlightingMarksReporter::run()
{
    run_internal();
    reportFinished();
}

void HighlightingMarksReporter::run_internal()
{
    if (isCanceled())
        return;

    for (const auto &highlightingMark : m_highlightingMarks)
        reportChunkWise(toHighlightingResult(highlightingMark));

    if (isCanceled())
        return;

    reportAndClearCurrentChunks();
}

QFuture<TextEditor::HighlightingResult> HighlightingMarksReporter::start()
{
    this->setRunnable(this);
    this->reportStarted();
    QFuture<TextEditor::HighlightingResult> future = this->future();
    QThreadPool::globalInstance()->start(this, QThread::LowestPriority);
    return future;
}

} // namespace ClangCodeModel
