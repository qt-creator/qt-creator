// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "astobjecttextextractor.h"

#include <qmljs/parser/qmljsast_p.h>

#include <QDebug>

using namespace QmlDesigner;

ASTObjectTextExtractor::ASTObjectTextExtractor(const QString &text)
    : m_document(QmlJS::Document::create(Utils::FilePath::fromString("<ASTObjectTextExtractor>"),
                                         QmlJS::Dialect::Qml))
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

void ASTObjectTextExtractor::throwRecursionDepthError()
{
    qWarning("Warning: Hit maximum recursion depth while visiting the AST in ASTObjectTextExtractor");
}
