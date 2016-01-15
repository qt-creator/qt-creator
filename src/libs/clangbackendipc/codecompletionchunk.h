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

#ifndef CLANGBACKEND_CODECOMPLETIONCHUNK_H
#define CLANGBACKEND_CODECOMPLETIONCHUNK_H

#include "clangbackendipc_global.h"

#include <utf8string.h>

#include <QVector>

namespace ClangBackEnd {

class CodeCompletionChunk;
using CodeCompletionChunks = QVector<CodeCompletionChunk>;

class CMBIPC_EXPORT CodeCompletionChunk
{
    friend CMBIPC_EXPORT QDataStream &operator<<(QDataStream &out, const CodeCompletionChunk &chunk);
    friend CMBIPC_EXPORT QDataStream &operator>>(QDataStream &in, CodeCompletionChunk &chunk);
    friend CMBIPC_EXPORT bool operator==(const CodeCompletionChunk &first, const CodeCompletionChunk &second);

public:
    enum Kind : quint8 {
        Optional,
        TypedText,
        Text,
        Placeholder,
        Informative,
        CurrentParameter,
        LeftParen,
        RightParen,
        LeftBracket,
        RightBracket,
        LeftBrace,
        RightBrace,
        LeftAngle,
        RightAngle,
        Comma,
        ResultType,
        Colon,
        SemiColon,
        Equal,
        HorizontalSpace,
        VerticalSpace,
        Invalid = 255
    };

    CodeCompletionChunk() = default;
    CodeCompletionChunk(Kind kind,
                        const Utf8String &text,
                        bool isOptional = false);

    Kind kind() const;
    const Utf8String &text() const;
    bool isOptional() const;

private:
    quint8 &kindAsInt();

private:
    Utf8String text_;
    Kind kind_ = Invalid;
    bool isOptional_ = false;
};

CMBIPC_EXPORT QDataStream &operator<<(QDataStream &out, const CodeCompletionChunk &chunk);
CMBIPC_EXPORT QDataStream &operator>>(QDataStream &in, CodeCompletionChunk &chunk);
CMBIPC_EXPORT bool operator==(const CodeCompletionChunk &first, const CodeCompletionChunk &second);

CMBIPC_EXPORT QDebug operator<<(QDebug debug, const CodeCompletionChunk &chunk);

void PrintTo(const CodeCompletionChunk &chunk, ::std::ostream* os);
void PrintTo(const CodeCompletionChunk::Kind &kind, ::std::ostream* os);

} // namespace ClangBackEnd

Q_DECLARE_METATYPE(ClangBackEnd::CodeCompletionChunk)

#endif // CLANGBACKEND_CODECOMPLETIONCHUNK_H
