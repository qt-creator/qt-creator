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
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
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
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "clangfunctionhintmodel.h"

#include "completionchunkstotextconverter.h"

#include <cplusplus/SimpleLexer.h>

namespace ClangCodeModel {
namespace Internal {

using namespace CPlusPlus;

ClangFunctionHintModel::ClangFunctionHintModel(const ClangBackEnd::CodeCompletions &functionSymbols)
    : m_functionSymbols(functionSymbols)
    , m_currentArg(-1)
{
}

void ClangFunctionHintModel::reset()
{
}

int ClangFunctionHintModel::size() const
{
    return m_functionSymbols.size();
}

QString ClangFunctionHintModel::text(int index) const
{
#if 0
    // TODO: add the boldening to the result
    Overview overview;
    overview.setShowReturnTypes(true);
    overview.setShowArgumentNames(true);
    overview.setMarkedArgument(m_currentArg + 1);
    Function *f = m_functionSymbols.at(index);

    const QString prettyMethod = overview(f->type(), f->name());
    const int begin = overview.markedArgumentBegin();
    const int end = overview.markedArgumentEnd();

    QString hintText;
    hintText += prettyMethod.left(begin).toHtmlEscaped());
    hintText += "<b>";
    hintText += prettyMethod.mid(begin, end - begin).toHtmlEscaped());
    hintText += "</b>";
    hintText += prettyMethod.mid(end).toHtmlEscaped());
    return hintText;
#endif
    return CompletionChunksToTextConverter::convertToFunctionSignature(m_functionSymbols.at(index).chunks());
}

int ClangFunctionHintModel::activeArgument(const QString &prefix) const
{
    int argnr = 0;
    int parcount = 0;
    SimpleLexer tokenize;
    Tokens tokens = tokenize(prefix);
    for (int i = 0; i < tokens.count(); ++i) {
        const Token &tk = tokens.at(i);
        if (tk.is(T_LPAREN))
            ++parcount;
        else if (tk.is(T_RPAREN))
            --parcount;
        else if (! parcount && tk.is(T_COMMA))
            ++argnr;
    }

    if (parcount < 0)
        return -1;

    if (argnr != m_currentArg)
        m_currentArg = argnr;

    return argnr;
}

} // namespace Internal
} // namespace ClangCodeModel

