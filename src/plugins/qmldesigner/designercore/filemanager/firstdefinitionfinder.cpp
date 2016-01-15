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

#include "firstdefinitionfinder.h"

#include <qmljs/parser/qmljsast_p.h>
#include <qmljs/parser/qmljsengine_p.h>

#include <QDebug>

using namespace QmlDesigner;

FirstDefinitionFinder::FirstDefinitionFinder(const QString &text):
        m_doc(QmlJS::Document::create(QLatin1String("<internal>"), QmlJS::Dialect::Qml))
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
    Finds the first object definition inside the object specified by \a offset.
    Returns the offset of the first object definition.
  */
qint32 FirstDefinitionFinder::operator()(quint32 offset)
{
    m_offset = offset;
    m_firstObjectDefinition = 0;

    QmlJS::AST::Node::accept(m_doc->qmlProgram(), this);

    if (!m_firstObjectDefinition)
        return -1;

    return m_firstObjectDefinition->firstSourceLocation().offset;
}

void FirstDefinitionFinder::extractFirstObjectDefinition(QmlJS::AST::UiObjectInitializer* ast)
{
    if (!ast)
        return;

    for (QmlJS::AST::UiObjectMemberList *iter = ast->members; iter; iter = iter->next) {
        if (QmlJS::AST::UiObjectDefinition *def = QmlJS::AST::cast<QmlJS::AST::UiObjectDefinition*>(iter->member))
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
