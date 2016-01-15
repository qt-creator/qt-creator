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

#include "ASTPath.h"

#include <cplusplus/AST.h>
#include <cplusplus/TranslationUnit.h>

#ifdef WITH_AST_PATH_DUMP
#  include <QDebug>
#  include <typeinfo>
#endif // WITH_AST_PATH_DUMP

using namespace CPlusPlus;

QList<AST *> ASTPath::operator()(int line, int column)
{
    _nodes.clear();
    _line = line;
    _column = column;

    if (_doc) {
        if (TranslationUnit *unit = _doc->translationUnit())
            accept(unit->ast());
    }

    return _nodes;
}

#ifdef WITH_AST_PATH_DUMP
void ASTPath::dump(const QList<AST *> nodes)
{
    qDebug() << "ASTPath dump," << nodes.size() << "nodes:";
    for (int i = 0; i < nodes.size(); ++i)
        qDebug() << qPrintable(QString(i + 1, QLatin1Char('-'))) << typeid(*nodes.at(i)).name();
}
#endif // WITH_AST_PATH_DUMP

bool ASTPath::preVisit(AST *ast)
{
    const unsigned firstToken = firstNonGeneratedToken(ast);
    const unsigned lastToken = lastNonGeneratedToken(ast);

    if (firstToken > 0) {
        if (lastToken <= firstToken)
            return false;

        unsigned startLine, startColumn;
        getTokenStartPosition(firstToken, &startLine, &startColumn);

        if (_line > startLine || (_line == startLine && _column >= startColumn)) {

            unsigned endLine, endColumn;
            getTokenEndPosition(lastToken - 1, &endLine, &endColumn);

            if (_line < endLine || (_line == endLine && _column <= endColumn)) {
                _nodes.append(ast);
                return true;
            }
        }
    }

    return false;
}

unsigned ASTPath::firstNonGeneratedToken(AST *ast) const
{
    const unsigned lastTokenIndex = ast->lastToken();
    unsigned tokenIndex = ast->firstToken();
    while (tokenIndex <= lastTokenIndex && tokenAt(tokenIndex).generated())
        ++tokenIndex;
    return tokenIndex;
}

unsigned ASTPath::lastNonGeneratedToken(AST *ast) const
{
    const unsigned firstTokenIndex = ast->firstToken();
    const unsigned lastTokenIndex = ast->lastToken();
    unsigned tokenIndex = lastTokenIndex;
    while (firstTokenIndex <= tokenIndex && tokenAt(tokenIndex).generated())
        --tokenIndex;
    return tokenIndex != lastTokenIndex
            ? tokenIndex + 1
            : tokenIndex;
}
