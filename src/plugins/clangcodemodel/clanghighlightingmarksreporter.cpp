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

#include <texteditor/textstyles.h>
#include <utils/qtcassert.h>

#include <QFuture>

namespace {

TextEditor::TextStyle toTextStyle(ClangBackEnd::HighlightingType type)
{
    using ClangBackEnd::HighlightingType;

    switch (type) {
    case HighlightingType::Keyword:
        return TextEditor::C_KEYWORD;
    case HighlightingType::Function:
        return TextEditor::C_FUNCTION;
    case HighlightingType::VirtualFunction:
        return TextEditor::C_VIRTUAL_METHOD;
    case HighlightingType::Type:
        return TextEditor::C_TYPE;
    case HighlightingType::PrimitiveType:
        return TextEditor::C_PRIMITIVE_TYPE;
    case HighlightingType::LocalVariable:
        return TextEditor::C_LOCAL;
    case HighlightingType::Field:
        return TextEditor::C_FIELD;
    case HighlightingType::GlobalVariable:
        return TextEditor::C_GLOBAL;
    case HighlightingType::Enumeration:
        return TextEditor::C_ENUMERATION;
    case HighlightingType::Label:
        return TextEditor::C_LABEL;
    case HighlightingType::Preprocessor:
    case HighlightingType::PreprocessorDefinition:
    case HighlightingType::PreprocessorExpansion:
        return TextEditor::C_PREPROCESSOR;
    case HighlightingType::Declaration:
        return TextEditor::C_DECLARATION;
    case HighlightingType::OutputArgument:
        return TextEditor::C_OUTPUT_ARGUMENT;
    case HighlightingType::Operator:
        return TextEditor::C_OPERATOR;
    case HighlightingType::Comment:
        return TextEditor::C_COMMENT;
    case HighlightingType::StringLiteral:
        return TextEditor::C_STRING;
    case HighlightingType::NumberLiteral:
        return TextEditor::C_NUMBER;
    case HighlightingType::Invalid:
        QTC_CHECK(false); // never called
        return TextEditor::C_TEXT;
    }

    Q_UNREACHABLE();
}

TextEditor::TextStyles toTextStyles(ClangBackEnd::HighlightingTypes types)
{
    TextEditor::TextStyles textStyles;
    textStyles.mixinStyles.initializeElements();

    textStyles.mainStyle = toTextStyle(types.mainHighlightingType);

    for (ClangBackEnd::HighlightingType type : types.mixinHighlightingTypes)
        textStyles.mixinStyles.push_back(toTextStyle(type));

    return textStyles;
}

TextEditor::HighlightingResult toHighlightingResult(
        const ClangBackEnd::HighlightingMarkContainer &highlightingMark)
{
    const auto textStyles = toTextStyles(highlightingMark.types());

    return TextEditor::HighlightingResult(highlightingMark.line(),
                                          highlightingMark.column(),
                                          highlightingMark.length(),
                                          textStyles);
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

    using ClangBackEnd::HighlightingType;

    for (const auto &highlightingMark : m_highlightingMarks) {
        const HighlightingType mainType = highlightingMark.types().mainHighlightingType;
        if (mainType == HighlightingType::StringLiteral)
            continue;

        reportChunkWise(toHighlightingResult(highlightingMark));
    }

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
