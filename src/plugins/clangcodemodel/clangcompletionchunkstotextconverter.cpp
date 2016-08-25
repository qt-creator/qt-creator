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

#include "clangcompletionchunkstotextconverter.h"

#include <algorithm>
#include <functional>

namespace ClangCodeModel {
namespace Internal {

void CompletionChunksToTextConverter::parseChunks(
        const ClangBackEnd::CodeCompletionChunks &codeCompletionChunks)
{
    m_text.clear();
    m_placeholderPositions.clear();

    m_codeCompletionChunks = codeCompletionChunks;

    addExtraVerticalSpaceBetweenBraces();

    std::for_each(m_codeCompletionChunks.cbegin(),
                  m_codeCompletionChunks.cend(),
                  [this] (const ClangBackEnd::CodeCompletionChunk &chunk)
    {
        parseDependendOnTheOptionalState(chunk);
        m_previousCodeCompletionChunk = chunk;
    });
}

void CompletionChunksToTextConverter::setAddPlaceHolderText(bool addPlaceHolderText)
{
    m_addPlaceHolderText = addPlaceHolderText;
}

void CompletionChunksToTextConverter::setAddPlaceHolderPositions(bool addPlaceHolderPositions)
{
    m_addPlaceHolderPositions = addPlaceHolderPositions;
}

void CompletionChunksToTextConverter::setAddResultType(bool addResultType)
{
    m_addResultType = addResultType;
}

void CompletionChunksToTextConverter::setAddSpaces(bool addSpaces)
{
    m_addSpaces = addSpaces;
}

void CompletionChunksToTextConverter::setAddExtraVerticalSpaceBetweenBraces(
        bool addExtraVerticalSpaceBetweenBraces)
{
    m_addExtraVerticalSpaceBetweenBraces = addExtraVerticalSpaceBetweenBraces;
}

void CompletionChunksToTextConverter::setEmphasizeOptional(bool emphasizeOptional)
{
    m_emphasizeOptional = emphasizeOptional;
}

void CompletionChunksToTextConverter::setAddOptional(bool addOptional)
{
    m_addOptional = addOptional;
}

void CompletionChunksToTextConverter::setPlaceHolderToEmphasize(int placeHolderNumber)
{
    m_placeHolderPositionToEmphasize = placeHolderNumber;
}

void CompletionChunksToTextConverter::setupForKeywords()
{
    setAddPlaceHolderPositions(true);
    setAddSpaces(true);
    setAddExtraVerticalSpaceBetweenBraces(true);
}

const QString &CompletionChunksToTextConverter::text() const
{
    return m_text;
}

const std::vector<int> &CompletionChunksToTextConverter::placeholderPositions() const
{
    return m_placeholderPositions;
}

bool CompletionChunksToTextConverter::hasPlaceholderPositions() const
{
    return m_placeholderPositions.size() > 0;
}

QString CompletionChunksToTextConverter::convertToFunctionSignatureWithHtml(
        const ClangBackEnd::CodeCompletionChunks &codeCompletionChunks,
        int parameterToEmphasize)
{
    CompletionChunksToTextConverter converter;
    converter.setAddPlaceHolderText(true);
    converter.setAddResultType(true);

    converter.setTextFormat(TextFormat::Html);
    converter.setAddOptional(true);
    converter.setEmphasizeOptional(true);

    converter.setAddPlaceHolderPositions(true);
    converter.setPlaceHolderToEmphasize(parameterToEmphasize);

    converter.parseChunks(codeCompletionChunks);

    return converter.text();
}

QString CompletionChunksToTextConverter::convertToName(
        const ClangBackEnd::CodeCompletionChunks &codeCompletionChunks)
{
    CompletionChunksToTextConverter converter;

    converter.parseChunks(codeCompletionChunks);

    return converter.text();
}

QString CompletionChunksToTextConverter::convertToToolTipWithHtml(
        const ClangBackEnd::CodeCompletionChunks &codeCompletionChunks)
{
    CompletionChunksToTextConverter converter;
    converter.setAddPlaceHolderText(true);
    converter.setAddSpaces(true);
    converter.setAddExtraVerticalSpaceBetweenBraces(true);
    converter.setAddOptional(true);
    converter.setTextFormat(TextFormat::Html);
    converter.setEmphasizeOptional(true);
    converter.setAddResultType(true);

    converter.parseChunks(codeCompletionChunks);

    return converter.text();
}

void CompletionChunksToTextConverter::parse(
        const ClangBackEnd::CodeCompletionChunk &codeCompletionChunk)
{
    using ClangBackEnd::CodeCompletionChunk;

    switch (codeCompletionChunk.kind()) {
        case CodeCompletionChunk::ResultType: parseResultType(codeCompletionChunk.text()); break;
        case CodeCompletionChunk::Placeholder: parsePlaceHolder(codeCompletionChunk); break;
        case CodeCompletionChunk::LeftParen: parseLeftParen(codeCompletionChunk); break;
        case CodeCompletionChunk::LeftBrace: parseLeftBrace(codeCompletionChunk); break;
        default: parseText(codeCompletionChunk.text()); break;
    }
}

void CompletionChunksToTextConverter::parseDependendOnTheOptionalState(
        const ClangBackEnd::CodeCompletionChunk &codeCompletionChunk)
{
    wrapInCursiveTagIfOptional(codeCompletionChunk);

    if (isNotOptionalOrAddOptionals(codeCompletionChunk))
        parse(codeCompletionChunk);
}

void CompletionChunksToTextConverter::parseResultType(const Utf8String &resultTypeText)
{
    if (m_addResultType)
        m_text += inDesiredTextFormat(resultTypeText) + QChar(QChar::Space);
}

void CompletionChunksToTextConverter::parseText(const Utf8String &text)
{
    if (canAddSpace()
            && m_previousCodeCompletionChunk.kind() == ClangBackEnd::CodeCompletionChunk::RightBrace) {
        m_text += QChar(QChar::Space);
    }

    m_text += inDesiredTextFormat(text);
}

void CompletionChunksToTextConverter::wrapInCursiveTagIfOptional(
        const ClangBackEnd::CodeCompletionChunk &codeCompletionChunk)
{
    if (m_addOptional) {
        if (m_emphasizeOptional && m_textFormat == TextFormat::Html) {
            if (!m_previousCodeCompletionChunk.isOptional() && codeCompletionChunk.isOptional())
                m_text += QStringLiteral("<i>");
            else if (m_previousCodeCompletionChunk.isOptional() && !codeCompletionChunk.isOptional())
                m_text += QStringLiteral("</i>");
        }
    }
}

void CompletionChunksToTextConverter::parsePlaceHolder(
        const ClangBackEnd::CodeCompletionChunk &codeCompletionChunk)
{
    if (m_addPlaceHolderText) {
        appendText(inDesiredTextFormat(codeCompletionChunk.text()),
                   emphasizeCurrentPlaceHolder());
    }

    if (m_addPlaceHolderPositions)
        m_placeholderPositions.push_back(m_text.size());
}

void CompletionChunksToTextConverter::parseLeftParen(
        const ClangBackEnd::CodeCompletionChunk &codeCompletionChunk)
{
    if (canAddSpace())
        m_text += QChar(QChar::Space);

    m_text += codeCompletionChunk.text().toString();
}

void CompletionChunksToTextConverter::parseLeftBrace(
        const ClangBackEnd::CodeCompletionChunk &codeCompletionChunk)
{
    if (canAddSpace())
        m_text += QChar(QChar::Space);

    m_text += codeCompletionChunk.text().toString();
}

void CompletionChunksToTextConverter::addExtraVerticalSpaceBetweenBraces()
{
    if (m_addExtraVerticalSpaceBetweenBraces)
        addExtraVerticalSpaceBetweenBraces(m_codeCompletionChunks.begin());
}

void CompletionChunksToTextConverter::addExtraVerticalSpaceBetweenBraces(
        const ClangBackEnd::CodeCompletionChunks::iterator &begin)
{
    using ClangBackEnd::CodeCompletionChunk;

    const auto leftBraceCompare = [] (const CodeCompletionChunk &chunk) {
        return chunk.kind() == CodeCompletionChunk::LeftBrace;
    };

    const auto rightBraceCompare = [] (const CodeCompletionChunk &chunk) {
        return chunk.kind() == CodeCompletionChunk::RightBrace;
    };

    const auto verticalSpaceCompare = [] (const CodeCompletionChunk &chunk) {
        return chunk.kind() == CodeCompletionChunk::VerticalSpace;
    };

    auto leftBrace = std::find_if(begin, m_codeCompletionChunks.end(), leftBraceCompare);

    if (leftBrace != m_codeCompletionChunks.end()) {
        auto rightBrace = std::find_if(leftBrace, m_codeCompletionChunks.end(), rightBraceCompare);

        if (rightBrace != m_codeCompletionChunks.end()) {
            auto verticalSpaceCount = std::count_if(leftBrace, rightBrace, verticalSpaceCompare);

            if (verticalSpaceCount <= 1) {
                auto distance = std::distance(leftBrace, rightBrace);
                CodeCompletionChunk verticalSpaceChunck(CodeCompletionChunk::VerticalSpace,
                                                        Utf8StringLiteral("\n"));
                auto verticalSpace = m_codeCompletionChunks.insert(std::next(leftBrace),
                                                                   verticalSpaceChunck);
                std::advance(verticalSpace, distance);
                rightBrace = verticalSpace;
            }

            auto begin = std::next(rightBrace);

            if (begin != m_codeCompletionChunks.end())
                addExtraVerticalSpaceBetweenBraces(begin);
        }
    }
}

QString CompletionChunksToTextConverter::inDesiredTextFormat(const Utf8String &text) const
{
    if (m_textFormat == TextFormat::Html)
        return text.toString().toHtmlEscaped();
    else
        return text.toString();
}

bool CompletionChunksToTextConverter::emphasizeCurrentPlaceHolder() const
{
    if (m_addPlaceHolderPositions) {
        const uint currentPlaceHolderPosition = uint(m_placeholderPositions.size() + 1);
        return uint(m_placeHolderPositionToEmphasize) == currentPlaceHolderPosition;
    }

    return false;
}

void CompletionChunksToTextConverter::setTextFormat(TextFormat textFormat)
{
    m_textFormat = textFormat;
}

void CompletionChunksToTextConverter::appendText(const QString &text, bool boldFormat)
{
    if (boldFormat && m_textFormat == TextFormat::Html)
        m_text += QStringLiteral("<b>") + text + QStringLiteral("</b>");
    else
        m_text += text;
}

bool CompletionChunksToTextConverter::canAddSpace() const
{
    return m_addSpaces
        && m_previousCodeCompletionChunk.kind() != ClangBackEnd::CodeCompletionChunk::HorizontalSpace
        && m_previousCodeCompletionChunk.kind() != ClangBackEnd::CodeCompletionChunk::RightAngle;
}

bool CompletionChunksToTextConverter::isNotOptionalOrAddOptionals(
        const ClangBackEnd::CodeCompletionChunk &codeCompletionChunk) const
{
    return !codeCompletionChunk.isOptional() || m_addOptional;
}

} // namespace Internal
} // namespace ClangCodeModel

