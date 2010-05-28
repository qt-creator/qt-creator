/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "ASTPath.h"

#include <AST.h>
#include <TranslationUnit.h>

#ifdef DEBUG_AST_PATH
#  include <QtCore/QDebug>
#  include <typeinfo>
#endif // DEBUG_AST_PATH

using namespace CPlusPlus;

QList<AST *> ASTPath::operator()(int line, int column)
{
    _nodes.clear();
    _line = line + 1;
    _column = column + 1;
    accept(_doc->translationUnit()->ast());
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

    if (firstToken > 0 && lastToken > firstToken) {
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
