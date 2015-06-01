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

#ifndef CLANGCODEMODEL_INTERNAL_COMPLETIONCHUNKSTOTEXTCONVERTER_H
#define CLANGCODEMODEL_INTERNAL_COMPLETIONCHUNKSTOTEXTCONVERTER_H

#include <codemodelbackendipc/codecompletionchunk.h>

#include <QString>

#include <sqlite/utf8string.h>

namespace ClangCodeModel {
namespace Internal {

class CompletionChunksToTextConverter
{
public:
    void parseChunks(const QVector<CodeModelBackEnd::CodeCompletionChunk> &codeCompletionChunks);

    const QString &text() const;

    static QString convert(const QVector<CodeModelBackEnd::CodeCompletionChunk> &codeCompletionChunks);

private:
    void parse(const CodeModelBackEnd::CodeCompletionChunk & codeCompletionChunk);
    void parseResultType(const Utf8String &text);
    void parseText(const Utf8String &text);
    void parseOptional(const CodeModelBackEnd::CodeCompletionChunk & optionalCodeCompletionChunk);

private:
    QString m_text;
};

} // namespace Internal
} // namespace ClangCodeModel

#endif // CLANGCODEMODEL_INTERNAL_COMPLETIONCHUNKSTOTEXTCONVERTER_H
