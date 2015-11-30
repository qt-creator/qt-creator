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
    enum Kind : quint32 {
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
        Invalid = 255};

    CodeCompletionChunk();
    CodeCompletionChunk(Kind kind,
                        const Utf8String &text,
                        const CodeCompletionChunks &optionalChunks = CodeCompletionChunks());

    Kind kind() const;
    const Utf8String &text() const;
    const CodeCompletionChunks &optionalChunks() const;

private:
    quint32 &kindAsInt();

private:
    Utf8String text_;
    CodeCompletionChunks optionalChunks_;
    Kind kind_ = Invalid;

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
