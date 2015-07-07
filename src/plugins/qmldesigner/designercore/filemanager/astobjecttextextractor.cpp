/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
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
