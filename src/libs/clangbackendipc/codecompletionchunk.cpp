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

#include "codecompletionchunk.h"

#include <QDataStream>
#include <QDebug>

#include <ostream>

namespace ClangBackEnd {

CodeCompletionChunk::CodeCompletionChunk()
    : kind_(Invalid)
{
}

CodeCompletionChunk::CodeCompletionChunk(CodeCompletionChunk::Kind kind,
                                         const Utf8String &text,
                                         const CodeCompletionChunks &optionalChunks)
    : text_(text),
      optionalChunks_(optionalChunks),
      kind_(kind)
{
}

CodeCompletionChunk::Kind CodeCompletionChunk::kind() const
{
    return kind_;
}

const Utf8String &CodeCompletionChunk::text() const
{
    return text_;
}

const CodeCompletionChunks &CodeCompletionChunk::optionalChunks() const
{
    return optionalChunks_;
}

quint32 &CodeCompletionChunk::kindAsInt()
{
    return reinterpret_cast<quint32&>(kind_);
}

QDataStream &operator<<(QDataStream &out, const CodeCompletionChunk &chunk)
{
    out << chunk.kind_;
    out << chunk.text_;

    if (chunk.kind() == CodeCompletionChunk::Optional)
        out << chunk.optionalChunks_;

    return out;
}

QDataStream &operator>>(QDataStream &in, CodeCompletionChunk &chunk)
{
    in >> chunk.kindAsInt();
    in >> chunk.text_;

    if (chunk.kind_ == CodeCompletionChunk::Optional)
        in >> chunk.optionalChunks_;

    return in;
}

bool operator==(const CodeCompletionChunk &first, const CodeCompletionChunk &second)
{
    return first.kind() == second.kind()
            && first.text() == second.text()
            && first.optionalChunks() == second.optionalChunks();
}

static const char *completionChunkKindToString(CodeCompletionChunk::Kind kind)
{
    switch (kind) {
       case CodeCompletionChunk::Optional: return "Optional";
       case CodeCompletionChunk::TypedText: return "TypedText";
       case CodeCompletionChunk::Text: return "Text";
       case CodeCompletionChunk::Placeholder: return "Placeholder";
       case CodeCompletionChunk::Informative: return "Informative";
       case CodeCompletionChunk::CurrentParameter: return "CurrentParameter";
       case CodeCompletionChunk::LeftParen: return "LeftParen";
       case CodeCompletionChunk::RightParen: return "RightParen";
       case CodeCompletionChunk::LeftBracket: return "LeftBracket";
       case CodeCompletionChunk::RightBracket: return "RightBracket";
       case CodeCompletionChunk::LeftBrace: return "LeftBrace";
       case CodeCompletionChunk::RightBrace: return "RightBrace";
       case CodeCompletionChunk::LeftAngle: return "LeftAngle";
       case CodeCompletionChunk::RightAngle: return "RightAngle";
       case CodeCompletionChunk::Comma: return "Comma";
       case CodeCompletionChunk::ResultType: return "ResultType";
       case CodeCompletionChunk::Colon: return "Colon";
       case CodeCompletionChunk::SemiColon: return "SemiColon";
       case CodeCompletionChunk::Equal: return "Equal";
       case CodeCompletionChunk::HorizontalSpace: return "HorizontalSpace";
       case CodeCompletionChunk::VerticalSpace: return "VerticalSpace";
       case CodeCompletionChunk::Invalid: return "Invalid";
    }

    return nullptr;
}

QDebug operator<<(QDebug debug, const CodeCompletionChunk &chunk)
{
    debug.nospace() << "CodeCompletionChunk(";
    debug.nospace() << completionChunkKindToString(chunk.kind()) << ", ";
    debug.nospace() << chunk.text();

    if (chunk.kind() == CodeCompletionChunk::Optional)
        debug.nospace() << ", " << chunk.optionalChunks();

    debug.nospace() << ")";

    return debug;
}

void PrintTo(const CodeCompletionChunk &chunk, ::std::ostream* os)
{
    *os << "{";
    *os << completionChunkKindToString(chunk.kind()) << ", ";
    *os << chunk.text().constData();

    if (chunk.kind() == CodeCompletionChunk::Optional) {
        const auto optionalChunks = chunk.optionalChunks();
        *os << ", {";

        for (auto optionalChunkPosition = optionalChunks.cbegin();
             optionalChunkPosition != optionalChunks.cend();
             ++optionalChunkPosition) {
            PrintTo(*optionalChunkPosition, os);
            if (std::next(optionalChunkPosition) != optionalChunks.cend())
                *os << ", ";
        }

        *os << "}";
    }

    *os << "}";
}

void PrintTo(const CodeCompletionChunk::Kind &kind, ::std::ostream* os)
{
    *os << completionChunkKindToString(kind);
}

} // namespace ClangBackEnd

