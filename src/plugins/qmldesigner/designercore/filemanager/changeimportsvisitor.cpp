/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include <qmljs/parser/qmljsast_p.h>
#include <qmljs/parser/qmljsengine_p.h>

#include "changeimportsvisitor.h"

using namespace QmlJS;
using namespace QmlJS::AST;

using namespace QmlDesigner;
using namespace QmlDesigner::Internal;

ChangeImportsVisitor::ChangeImportsVisitor(TextModifier &textModifier,
                                           const QString &source):
        QMLRewriter(textModifier), m_source(source)
{}

bool ChangeImportsVisitor::add(QmlJS::AST::UiProgram *ast, const Import &import)
{
    setDidRewriting(false);
    if (!ast)
        return false;

    if (ast->imports && ast->imports->import) {
        int insertionPoint = 0;
        if (ast->members && ast->members->member)
            insertionPoint = ast->members->member->firstSourceLocation().begin();
        else
            insertionPoint = m_source.length();
        while (insertionPoint > 0) {
            --insertionPoint;
            const QChar c = m_source.at(insertionPoint);
            if (!c.isSpace() && c != QLatin1Char(';'))
                break;
        }
        replace(insertionPoint+1, 0, QLatin1String("\n") + import.toString(false));
    } else {
        replace(0, 0, import.toString(false) + QLatin1String("\n\n"));
    }

    setDidRewriting(true);

    return true;
}

bool ChangeImportsVisitor::remove(QmlJS::AST::UiProgram *ast, const Import &import)
{
    setDidRewriting(false);
    if (!ast)
        return false;

    for (UiImportList *iter = ast->imports; iter; iter = iter->next) {
        if (equals(iter->import, import)) {
            int start = iter->firstSourceLocation().begin();
            int end = iter->lastSourceLocation().end();
            includeSurroundingWhitespace(start, end);
            replace(start, end - start, QString());
            setDidRewriting(true);
        }
    }

    return didRewriting();
}

bool ChangeImportsVisitor::equals(QmlJS::AST::UiImport *ast, const Import &import)
{
    if (import.isLibraryImport())
        return toString(ast->importUri) == import.url();
    else if (import.isFileImport())
        return ast->fileName == import.file();
    else
        return false;
}
