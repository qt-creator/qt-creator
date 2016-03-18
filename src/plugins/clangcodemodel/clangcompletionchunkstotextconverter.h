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

#include <clangbackendipc/codecompletionchunk.h>

#include <sqlite/utf8string.h>

#include <QString>

#include <vector>

namespace ClangCodeModel {
namespace Internal {

class CompletionChunksToTextConverter
{
public:
    void parseChunks(const ClangBackEnd::CodeCompletionChunks &codeCompletionChunks);

    enum class TextFormat {
        Plain,
        Html
    };
    void setTextFormat(TextFormat textFormat);
    void setAddPlaceHolderText(bool addPlaceHolderText);
    void setAddPlaceHolderPositions(bool addPlaceHolderPositions);
    void setAddResultType(bool addResultType);
    void setAddSpaces(bool addSpaces);
    void setAddExtraVerticalSpaceBetweenBraces(bool addExtraVerticalSpaceBetweenBraces);
    void setEmphasizeOptional(bool emphasizeOptional); // Only for Html format
    void setAddOptional(bool addOptional);
    void setPlaceHolderToEmphasize(int placeHolderNumber);

    void setupForKeywords();

    const QString &text() const;
    const std::vector<int> &placeholderPositions() const;
    bool hasPlaceholderPositions() const;

    static QString convertToName(const ClangBackEnd::CodeCompletionChunks &codeCompletionChunks);
    static QString convertToKeywords(const ClangBackEnd::CodeCompletionChunks &codeCompletionChunks);
    static QString convertToToolTipWithHtml(const ClangBackEnd::CodeCompletionChunks &codeCompletionChunks);
    static QString convertToFunctionSignatureWithHtml(
            const ClangBackEnd::CodeCompletionChunks &codeCompletionChunks,
            int parameterToEmphasize = -1);

private:
    void parse(const ClangBackEnd::CodeCompletionChunk &codeCompletionChunk);
    void parseDependendOnTheOptionalState(const ClangBackEnd::CodeCompletionChunk &codeCompletionChunk);
    void parseResultType(const Utf8String &text);
    void parseText(const Utf8String &text);
    void wrapInCursiveTagIfOptional(const ClangBackEnd::CodeCompletionChunk &codeCompletionChunk);
    void parsePlaceHolder(const ClangBackEnd::CodeCompletionChunk &codeCompletionChunk);
    void parseLeftParen(const ClangBackEnd::CodeCompletionChunk &codeCompletionChunk);
    void parseLeftBrace(const ClangBackEnd::CodeCompletionChunk &codeCompletionChunk);
    void addExtraVerticalSpaceBetweenBraces();
    void addExtraVerticalSpaceBetweenBraces(const ClangBackEnd::CodeCompletionChunks::iterator &);

    QString inDesiredTextFormat(const Utf8String &text) const;
    void appendText(const QString &text, bool boldFormat = false); // Boldness only in Html format
    bool canAddSpace() const;
    bool isNotOptionalOrAddOptionals(const ClangBackEnd::CodeCompletionChunk &codeCompletionChunk) const;

    bool emphasizeCurrentPlaceHolder() const;

private:
    std::vector<int> m_placeholderPositions;
    ClangBackEnd::CodeCompletionChunks m_codeCompletionChunks;
    ClangBackEnd::CodeCompletionChunk m_previousCodeCompletionChunk;
    QString m_text;
    int m_placeHolderPositionToEmphasize = -1;
    TextFormat m_textFormat = TextFormat::Plain;
    bool m_addPlaceHolderText = false;
    bool m_addPlaceHolderPositions = false;
    bool m_addResultType = false;
    bool m_addSpaces = false;
    bool m_addExtraVerticalSpaceBetweenBraces = false;
    bool m_emphasizeOptional = false;
    bool m_addOptional = false;
};

} // namespace Internal
} // namespace ClangCodeModel
