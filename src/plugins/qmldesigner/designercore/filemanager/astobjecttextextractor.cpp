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

#include "astobjecttextextractor.h"

#include <qmljs/parser/qmljsast_p.h>

using namespace QmlDesigner;

ASTObjectTextExtractor::ASTObjectTextExtractor(const QString &text):
        m_document(QmlJS::Document::create(QLatin1String("<ASTObjectTextExtractor>"), QmlJS::Dialect::Qml))
{
    m_document->setSource(text);
    m_document->parseQml();
}

QString ASTObjectTextExtractor::operator ()(int location)
{
    Q_ASSERT(location >= 0);

    m_location = location;
    m_text.clear();

    QmlJS::AST::Node::accept(m_document->qmlProgram(), this);

    return m_text;
}

bool ASTObjectTextExtractor::visit(QmlJS::AST::UiObjectBinding *ast)
{
    if (!m_text.isEmpty())
        return false;

    if (ast->qualifiedTypeNameId->identifierToken.offset == m_location)
        m_text = m_document->source().mid(m_location, ast->lastSourceLocation().end() - m_location);

    return m_text.isEmpty();
}

bool ASTObjectTextExtractor::visit(QmlJS::AST::UiObjectDefinition *ast)
{
    if (!m_text.isEmpty())
        return false;

    if (ast->firstSourceLocation().offset == m_location)
        m_text = m_document->source().mid(m_location, ast->lastSourceLocation().end() - m_location);

    return m_text.isEmpty();
}
