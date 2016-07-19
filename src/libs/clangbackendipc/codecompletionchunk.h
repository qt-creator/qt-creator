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

#include "clangbackendipc_global.h"

#include <utf8string.h>

#include <QDataStream>
#include <QVector>

namespace ClangBackEnd {

class CodeCompletionChunk;
using CodeCompletionChunks = QVector<CodeCompletionChunk>;

class CodeCompletionChunk
{
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
                        bool isOptional = false)
        : text_(text),
          kind_(kind),
          isOptional_(isOptional)
    {
    }

    Kind kind() const
    {
        return kind_;
    }

    const Utf8String &text() const
    {
        return text_;
    }

    bool isOptional() const
    {
        return isOptional_;
    }

    friend QDataStream &operator<<(QDataStream &out, const CodeCompletionChunk &chunk)
    {
        out << static_cast<quint8>(chunk.kind_);
        out << chunk.text_;
        out << chunk.isOptional_;

        return out;
    }

    friend QDataStream &operator>>(QDataStream &in, CodeCompletionChunk &chunk)
    {
        quint8 kind;

        in >> kind;
        in >> chunk.text_;
        in >> chunk.isOptional_;

        chunk.kind_ = static_cast<CodeCompletionChunk::Kind>(kind);

        return in;
    }

    friend bool operator==(const CodeCompletionChunk &first, const CodeCompletionChunk &second)
    {
        return first.kind() == second.kind()
                && first.text() == second.text()
                && first.isOptional() == second.isOptional();
    }

private:
    Utf8String text_;
    Kind kind_ = Invalid;
    bool isOptional_ = false;
};

CMBIPC_EXPORT QDebug operator<<(QDebug debug, const CodeCompletionChunk &chunk);

void PrintTo(const CodeCompletionChunk &chunk, ::std::ostream* os);
void PrintTo(const CodeCompletionChunk::Kind &kind, ::std::ostream* os);

} // namespace ClangBackEnd
