/****************************************************************************
**
** Copyright (C) 2015 Digia Plc and/or its subsidiary(-ies).
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

#include "completionchunkstotextconverter.h"

#include <algorithm>
#include <functional>

namespace ClangCodeModel {
namespace Internal {

void CompletionChunksToTextConverter::parseChunks(const ClangBackEnd::CodeCompletionChunks &codeCompletionChunks)
{
    m_text.clear();
    m_placeholderPositions.clear();

    m_codeCompletionChunks = codeCompletionChunks;

    addExtraVerticalSpaceBetweenBraces();

    std::for_each(m_codeCompletionChunks.cbegin(),
                  m_codeCompletionChunks.cend(),
                  [this] (const ClangBackEnd::CodeCompletionChunk &chunk)
    {
        parse(chunk);
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

void CompletionChunksToTextConverter::setAddExtraVerticalSpaceBetweenBraces(bool addExtraVerticalSpaceBetweenBraces)
{
    m_addExtraVerticalSpaceBetweenBraces = addExtraVerticalSpaceBetweenBraces;
}

void CompletionChunksToTextConverter::setAddHtmlTags(bool addHtmlTags)
{
    m_addHtmlTags = addHtmlTags;
}

void CompletionChunksToTextConverter::setAddOptional(bool addOptional)
{
    m_addOptional = addOptional;
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

QString CompletionChunksToTextConverter::convertToFunctionSignature(const ClangBackEnd::CodeCompletionChunks &codeCompletionChunks)
{
    CompletionChunksToTextConverter converter;
    converter.setAddPlaceHolderText(true);
    converter.setAddResultType(true);
    converter.setAddOptional(true);

    converter.parseChunks(codeCompletionChunks);

    return converter.text();
}

QString CompletionChunksToTextConverter::convertToName(const ClangBackEnd::CodeCompletionChunks &codeCompletionChunks)
{
    CompletionChunksToTextConverter converter;

    converter.parseChunks(codeCompletionChunks);

    return converter.text();
}

QString CompletionChunksToTextConverter::convertToToolTip(const ClangBackEnd::CodeCompletionChunks &codeCompletionChunks)
{
    CompletionChunksToTextConverter converter;
    converter.setAddPlaceHolderText(true);
    converter.setAddSpaces(true);
    converter.setAddExtraVerticalSpaceBetweenBraces(true);
    converter.setAddOptional(true);
    converter.setAddHtmlTags(true);
    converter.setAddResultType(true);

    converter.parseChunks(codeCompletionChunks);

    return converter.text();
}

void CompletionChunksToTextConverter::parse(const ClangBackEnd::CodeCompletionChunk &codeCompletionChunk)
{
    using ClangBackEnd::CodeCompletionChunk;

    switch (codeCompletionChunk.kind()) {
        case CodeCompletionChunk::ResultType: parseResultType(codeCompletionChunk.text()); break;
        case CodeCompletionChunk::Optional: parseOptional(codeCompletionChunk); break;
        case CodeCompletionChunk::Placeholder: parsePlaceHolder(codeCompletionChunk); break;
        case CodeCompletionChunk::LeftParen: parseLeftParen(codeCompletionChunk); break;
        case CodeCompletionChunk::LeftBrace: parseLeftBrace(codeCompletionChunk); break;
        default: parseText(codeCompletionChunk.text()); break;
    }
}

void CompletionChunksToTextConverter::parseResultType(const Utf8String &resultTypeText)
{
    if (m_addResultType)
        m_text += resultTypeText.toString() + QChar(QChar::Space);
}

void CompletionChunksToTextConverter::parseText(const Utf8String &text)
{
    if (canAddSpace()
            && m_previousCodeCompletionChunk.kind() == ClangBackEnd::CodeCompletionChunk::RightBrace) {
        m_text += QChar(QChar::Space);
    }

    m_text += text.toString();
}

void CompletionChunksToTextConverter::parseOptional(const ClangBackEnd::CodeCompletionChunk &optionalCodeCompletionChunk)
{
    if (m_addOptional) {
        if (m_addHtmlTags)
            m_text += QStringLiteral("<i>");

        m_text += convertToFunctionSignature(optionalCodeCompletionChunk.optionalChunks());

        if (m_addHtmlTags)
            m_text += QStringLiteral("</i>");
    }
}

void CompletionChunksToTextConverter::parsePlaceHolder(const ClangBackEnd::CodeCompletionChunk &codeCompletionChunk)
{
    if (m_addPlaceHolderText)
        m_text += codeCompletionChunk.text().toString();

    if (m_addPlaceHolderPositions)
        m_placeholderPositions.push_back(m_text.size());
}

void CompletionChunksToTextConverter::parseLeftParen(const ClangBackEnd::CodeCompletionChunk &codeCompletionChunk)
{
    if (canAddSpace())
        m_text += QChar(QChar::Space);

    m_text += codeCompletionChunk.text().toString();
}

void CompletionChunksToTextConverter::parseLeftBrace(const ClangBackEnd::CodeCompletionChunk &codeCompletionChunk)
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

void CompletionChunksToTextConverter::addExtraVerticalSpaceBetweenBraces(const ClangBackEnd::CodeCompletionChunks::iterator &begin)
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

bool CompletionChunksToTextConverter::canAddSpace() const
{
    return m_addSpaces
        && m_previousCodeCompletionChunk.kind() != ClangBackEnd::CodeCompletionChunk::HorizontalSpace
        && m_previousCodeCompletionChunk.kind() != ClangBackEnd::CodeCompletionChunk::RightAngle;
}

} // namespace Internal
} // namespace ClangCodeModel

