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

#include "firstdefinitionfinder.h"

#include <qmljs/parser/qmljsast_p.h>

#include <QDebug>

using namespace QmlJS;
using namespace QmlDesigner;
using namespace QmlJS::AST;

FirstDefinitionFinder::FirstDefinitionFinder(const QString &text):
        m_doc(Document::create("<internal>", Document::QmlLanguage))
{
    m_doc->setSource(text);
    bool ok = m_doc->parseQml();

    if (!ok) {
        qDebug() << text;
        foreach (const QmlJS::DiagnosticMessage &message, m_doc->diagnosticMessages())
                qDebug() << message.message;
    }

    Q_ASSERT(ok);
}

/*!
  \brief Finds the first object definition inside the object given by offset


  \arg the offset of the object to search in
  \return the offset of the first object definition
  */
qint32 FirstDefinitionFinder::operator()(quint32 offset)
{
    m_offset = offset;
    m_firstObjectDefinition = 0;

    Node::accept(m_doc->qmlProgram(), this);

    if (!m_firstObjectDefinition)
        return -1;

    return m_firstObjectDefinition->firstSourceLocation().offset;
}

void FirstDefinitionFinder::extractFirstObjectDefinition(UiObjectInitializer* ast)
{
    if (!ast)
        return;

    for (UiObjectMemberList *iter = ast->members; iter; iter = iter->next) {
        if (UiObjectDefinition *def = cast<UiObjectDefinition*>(iter->member))
            m_firstObjectDefinition = def;
    }
}

bool FirstDefinitionFinder::visit(QmlJS::AST::UiObjectBinding *ast)
{
    if (ast->qualifiedTypeNameId && ast->qualifiedTypeNameId->identifierToken.isValid()) {
        const quint32 start = ast->qualifiedTypeNameId->identifierToken.offset;

        if (start == m_offset) {
            extractFirstObjectDefinition(ast->initializer);
            return false;
        }
    }
    return true;
}

bool FirstDefinitionFinder::visit(QmlJS::AST::UiObjectDefinition *ast)
{
    const quint32 start = ast->firstSourceLocation().offset;

    if (start == m_offset) {
        extractFirstObjectDefinition(ast->initializer);
        return false;
    }
    return true;
}
