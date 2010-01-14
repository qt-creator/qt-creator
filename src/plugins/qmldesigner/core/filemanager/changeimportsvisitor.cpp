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

#include <qmljsast_p.h>
#include <qmljsengine_p.h>

#include "changeimportsvisitor.h"

using namespace QmlJS;
using namespace QmlJS::AST;

using namespace QmlDesigner;
using namespace QmlDesigner::Internal;
using namespace QmlDesigner::Internal;

ChangeImportsVisitor::ChangeImportsVisitor(TextModifier &textModifier, const QSet<Import> &addedImports, const QSet<Import> &removedImports, const QString &source):
        QMLRewriter(textModifier),
        CopyPasteUtil(source),
        m_addedImports(addedImports),
        m_removedImports(removedImports)
{
}

bool ChangeImportsVisitor::visit(QmlJS::AST::UiProgram *ast)
{
    if (ast->imports)
        accept(ast->imports);

    return false;
}

bool ChangeImportsVisitor::visit(QmlJS::AST::UiImportList *ast)
{
    if (!ast)
        return false;

    quint32 prevEnd = 0;
    for (UiImportList *it = ast; it; it = it->next) {
        UiImport *imp = it->import;
        if (!imp)
            continue;

        if (m_removedImports.remove(createImport(imp)))
            replace(prevEnd, imp->lastSourceLocation().end() - prevEnd, "");

        prevEnd = imp->lastSourceLocation().end();
    }

    foreach (const Import &i, m_addedImports) {
        replace(prevEnd, 0, i.toString(false) + "\n");
    }

    return false;
}
