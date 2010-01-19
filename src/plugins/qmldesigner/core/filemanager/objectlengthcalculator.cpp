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

#include "objectlengthcalculator.h"

#include <qmljs/parser/qmljsast_p.h>

using namespace QmlJS;
using namespace QmlDesigner;
using namespace QmlJS::AST;

ObjectLengthCalculator::ObjectLengthCalculator(const QString &text):
        m_doc(Document::create("<internal>"))
{
    m_doc->setSource(text);
    bool ok = m_doc->parseQml();

    Q_ASSERT(ok);
}

quint32 ObjectLengthCalculator::operator()(quint32 offset)
{
    m_offset = offset;
    m_length = 0;

    Node::accept(m_doc->qmlProgram(), this);

    return m_length;
}

bool ObjectLengthCalculator::visit(QmlJS::AST::UiObjectBinding *ast)
{
    if (m_length > 0)
        return false;

    if (ast->qualifiedTypeNameId && ast->qualifiedTypeNameId->identifierToken.isValid()) {
        const quint32 start = ast->qualifiedTypeNameId->identifierToken.offset;
        const quint32 end = ast->lastSourceLocation().end();

        if (start == m_offset) {
            m_length = end - start;
            return false;
        }

        return m_offset < end;
    } else {
        return true;
    }
}

bool ObjectLengthCalculator::visit(QmlJS::AST::UiObjectDefinition *ast)
{
    if (m_length > 0)
        return false;

    const quint32 start = ast->firstSourceLocation().offset;
    const quint32 end = ast->lastSourceLocation().end();

    if (start == m_offset) {
        m_length = end - start;
        return false;
    }

    return m_offset < end;
}
