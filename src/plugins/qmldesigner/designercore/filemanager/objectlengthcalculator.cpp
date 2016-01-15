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

#include "objectlengthcalculator.h"

#include <qmljs/parser/qmljsast_p.h>

using namespace QmlDesigner;

ObjectLengthCalculator::ObjectLengthCalculator():
        m_doc(QmlJS::Document::create(QLatin1String("<internal>"), QmlJS::Dialect::Qml))
{
}

bool ObjectLengthCalculator::operator()(const QString &text, quint32 offset,
                                        quint32 &length)
{
    m_offset = offset;
    m_length = 0;
    m_doc->setSource(text);

    if (!m_doc->parseQml())
        return false;

    QmlJS::AST::Node::accept(m_doc->qmlProgram(), this);
    if (m_length) {
        length = m_length;
        return true;
    } else {
        return false;
    }
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
