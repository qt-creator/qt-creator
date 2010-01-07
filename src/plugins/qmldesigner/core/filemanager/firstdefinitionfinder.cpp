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

#include "firstdefinitionfinder.h"

#include <qmljsast_p.h>

using namespace QmlDesigner;
using namespace QmlEditor;
using namespace QmlJS::AST;

FirstDefinitionFinder::FirstDefinitionFinder(const QString &text):
        m_doc(QmlDocument::create("<internal>"))
{
    m_doc->setSource(text);
    bool ok = m_doc->parse();

    Q_ASSERT(ok);
}

/*!
  \brief Finds the first object definition inside the object given by offset


  \arg the offset of the object to search in
  \return the offset of the first object definition
  */
quint32 FirstDefinitionFinder::operator()(quint32 offset)
{
    m_offset = offset;
    m_firstObjectDefinition = 0;

    Node::accept(m_doc->program(), this);

    return m_firstObjectDefinition->firstSourceLocation().offset;
}

void FirstDefinitionFinder::extractFirstObjectDefinition(UiObjectInitializer* ast)
{
    if (!ast)
        return;

    for (UiObjectMemberList *iter = ast->members; iter; iter = iter->next) {
        if (UiObjectDefinition *def = cast<UiObjectDefinition*>(iter->member)) {
            m_firstObjectDefinition = def;
        }
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
