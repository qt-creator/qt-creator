/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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

#include "qmlcodeformatter.h"
#include "qmljsast_p.h"

using namespace QmlEditor::Internal;
using namespace QmlJS;
using namespace QmlJS::AST;

QmlCodeFormatter::QmlCodeFormatter()
{
}

QmlCodeFormatter::~QmlCodeFormatter()
{
}

bool QmlCodeFormatter::visit(QmlJS::AST::UiProgram *ast)
{
    Node::accept(ast->imports, this);

    if (ast->imports && ast->members)
        newline();

    Node::accept(ast->members, this);

    return false;
}

QString QmlCodeFormatter::operator()(QmlJS::AST::UiProgram *ast, const QString &originalSource, const QList<QmlJS::AST::SourceLocation> & /* comments */, int start, int end)
{
    m_result.clear();
    m_result.reserve(originalSource.length() * 2);
    m_originalSource = originalSource;
    m_start = start;
    m_end = end;

    Node::acceptChild(ast, this);

    return m_result;
}

bool QmlCodeFormatter::visit(UiImport *ast)
{
    append("import ");
    append(ast->fileNameToken);

    if (ast->versionToken.isValid()) {
        append(' ');
        append(ast->versionToken);
    }

    if (ast->asToken.isValid()) {
        append(" as ");
        append(ast->importIdToken);
    }

    if (ast->semicolonToken.isValid())
        append(';');

    newline();

    return false;
}

bool QmlCodeFormatter::visit(UiObjectDefinition *ast)
{
    indent();
    Node::accept(ast->qualifiedTypeNameId, this);
    append(' ');
    Node::accept(ast->initializer, this);
    newline();

    return false;
}

bool QmlCodeFormatter::visit(QmlJS::AST::UiQualifiedId *ast)
{
    for (UiQualifiedId *it = ast; it; it = it->next) {
        append(it->name->asString());

        if (it->next)
            append('.');
    }

    return false;
}

bool QmlCodeFormatter::visit(QmlJS::AST::UiObjectInitializer *ast)
{
    append(ast->lbraceToken.offset, ast->rbraceToken.end() - ast->lbraceToken.offset);

    return false;
}
