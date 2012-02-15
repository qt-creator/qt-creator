/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "ASTPath.h"

#include <AST.h>
#include <TranslationUnit.h>

#ifdef DEBUG_AST_PATH
#  include <QDebug>
#  include <typeinfo>
#endif // DEBUG_AST_PATH

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

#ifdef DEBUG_AST_PATH
void ASTPath::dump(const QList<AST *> nodes)
{
    qDebug() << "ASTPath dump," << nodes.size() << "nodes:";
    for (int i = 0; i < nodes.size(); ++i)
        qDebug() << qPrintable(QString(i + 1, QLatin1Char('-'))) << typeid(*nodes.at(i)).name();
}
#endif // DEBUG_AST_PATH

bool ASTPath::preVisit(AST *ast)
{
    unsigned firstToken = ast->firstToken();
    unsigned lastToken = ast->lastToken();

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
