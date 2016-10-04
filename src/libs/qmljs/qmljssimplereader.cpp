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

#include "qmljssimplereader.h"

#include "parser/qmljsparser_p.h"
#include "parser/qmljslexer_p.h"
#include "parser/qmljsengine_p.h"

#include "qmljsutils.h"

#include <QFile>
#include <QLoggingCategory>

static Q_LOGGING_CATEGORY(simpleReaderLog, "qtc.qmljs.simpleReader")

namespace QmlJS{

QVariant SimpleReaderNode::property(const QString &name) const
{
    return m_properties.value(name);
}

QStringList SimpleReaderNode::propertyNames() const
{
    return m_properties.keys();
}

SimpleReaderNode::PropertyHash SimpleReaderNode::properties() const
{
    return m_properties;
}

bool SimpleReaderNode::isRoot() const
{
    return m_parentNode.isNull();
}

bool SimpleReaderNode::isValid() const
{
    return !m_name.isEmpty();
}

SimpleReaderNode::Ptr SimpleReaderNode::invalidNode()
{
    return Ptr(new SimpleReaderNode);
}

SimpleReaderNode::WeakPtr SimpleReaderNode::parent() const
{
    return m_parentNode;
}

QString SimpleReaderNode::name() const
{
    return m_name;
}

SimpleReaderNode::SimpleReaderNode()
{
}

SimpleReaderNode::SimpleReaderNode(const QString &name, WeakPtr parent)
    : m_name(name), m_parentNode(parent)
{
}

SimpleReaderNode::Ptr SimpleReaderNode::create(const QString &name, WeakPtr parent)
{
    Ptr newNode(new SimpleReaderNode(name, parent));
    newNode->m_weakThis = newNode;
    if (parent)
        parent.data()->m_children.append(newNode);
    return newNode;
}

const SimpleReaderNode::List SimpleReaderNode::children() const
{
    return m_children;
}

void SimpleReaderNode::setProperty(const QString &name, const QVariant &value)
{
    m_properties.insert(name, value);
}

SimpleAbstractStreamReader::SimpleAbstractStreamReader()
{
}

bool SimpleAbstractStreamReader::readFile(const QString &fileName)
{
    QFile file(fileName);
    if (file.open(QIODevice::ReadOnly)) {
        QByteArray source = file.readAll();
        file.close();
        return readFromSource(QString::fromLocal8Bit(source));
    }
    addError(tr("Cannot find file %1.").arg(fileName));
    return false;
}

bool SimpleAbstractStreamReader::readFromSource(const QString &source)
{
    m_errors.clear();
    m_currentSourceLocation = AST::SourceLocation();

    m_source = source;

    Engine engine;
    Lexer lexer(&engine);
    Parser parser(&engine);

    lexer.setCode(source, /*line = */ 1, /*qmlMode = */true);

    if (!parser.parse()) {
        QString errorMessage = QString::fromLatin1("%1:%2: %3").arg(
                    QString::number(parser.errorLineNumber()),
                    QString::number(parser.errorColumnNumber()),
                    parser.errorMessage());
        addError(errorMessage);
        return false;
    }
    return readDocument(parser.ast());
}

QStringList SimpleAbstractStreamReader::errors() const
{
    return m_errors;
}

void SimpleAbstractStreamReader::addError(const QString &error, const AST::SourceLocation &sourceLocation)
{
    m_errors << QString::fromLatin1("%1:%2: %3\n").arg(
                    QString::number(sourceLocation.startLine),
                    QString::number(sourceLocation.startColumn),
                    error);
}

AST::SourceLocation SimpleAbstractStreamReader::currentSourceLocation() const
{
    return m_currentSourceLocation;
}

bool SimpleAbstractStreamReader::readDocument(AST::UiProgram *ast)
{
    if (!ast) {
        addError(tr("Could not parse document."));
        return false;
    }

    AST::UiObjectDefinition *uiObjectDefinition = AST::cast<AST::UiObjectDefinition *>(ast->members->member);
    if (!uiObjectDefinition) {
        addError(tr("Expected document to contain a single object definition."));
        return false;
    }
    readChild(uiObjectDefinition);

    m_source.clear();

    return errors().isEmpty();
}

void SimpleAbstractStreamReader::readChildren(AST::UiObjectDefinition *uiObjectDefinition)
{
    Q_ASSERT(uiObjectDefinition);

    for (AST::UiObjectMemberList *it = uiObjectDefinition->initializer->members; it; it = it->next) {
        AST::UiObjectMember *member = it->member;
        AST::UiObjectDefinition *uiObjectDefinition = AST::cast<AST::UiObjectDefinition *>(member);
        if (uiObjectDefinition)
            readChild(uiObjectDefinition);
    }
}

void SimpleAbstractStreamReader::readChild(AST::UiObjectDefinition *uiObjectDefinition)
{
    Q_ASSERT(uiObjectDefinition);

    setSourceLocation(uiObjectDefinition->firstSourceLocation());

    elementStart(toString(uiObjectDefinition->qualifiedTypeNameId));

    readProperties(uiObjectDefinition);
    readChildren(uiObjectDefinition);

    elementEnd();
}

void SimpleAbstractStreamReader::readProperties(AST::UiObjectDefinition *uiObjectDefinition)
{
    Q_ASSERT(uiObjectDefinition);

    for (AST::UiObjectMemberList *it = uiObjectDefinition->initializer->members; it; it = it->next) {
          AST::UiObjectMember *member = it->member;
          AST::UiScriptBinding *scriptBinding = AST::cast<AST::UiScriptBinding *>(member);
          if (scriptBinding)
              readProperty(scriptBinding);
    }
}

void SimpleAbstractStreamReader::readProperty(AST::UiScriptBinding *uiScriptBinding)
{
    Q_ASSERT(uiScriptBinding);

    setSourceLocation(uiScriptBinding->firstSourceLocation());

    const QString name = toString(uiScriptBinding->qualifiedId);
    const QVariant value = parsePropertyScriptBinding(uiScriptBinding);

    propertyDefinition(name, value);
}

QVariant SimpleAbstractStreamReader::parsePropertyScriptBinding(AST::UiScriptBinding *uiScriptBinding)
{
    Q_ASSERT(uiScriptBinding);

    AST::ExpressionStatement *expStmt = AST::cast<AST::ExpressionStatement *>(uiScriptBinding->statement);
    if (!expStmt) {
        addError(tr("Expected expression statement after colon."), uiScriptBinding->statement->firstSourceLocation());
        return QVariant();
    }

    return parsePropertyExpression(expStmt->expression);
}

QVariant SimpleAbstractStreamReader::parsePropertyExpression(AST::ExpressionNode *expressionNode)
{
    Q_ASSERT(expressionNode);

    AST::ArrayLiteral *arrayLiteral = AST::cast<AST::ArrayLiteral *>(expressionNode);

    if (arrayLiteral) {
        QList<QVariant> variantList;
        for (AST::ElementList *it = arrayLiteral->elements; it; it = it->next)
            variantList << parsePropertyExpression(it->expression);
        return variantList;
    }

    AST::StringLiteral *stringLiteral = AST::cast<AST::StringLiteral *>(expressionNode);
    if (stringLiteral)
        return stringLiteral->value.toString();

    AST::TrueLiteral *trueLiteral = AST::cast<AST::TrueLiteral *>(expressionNode);
    if (trueLiteral)
        return true;

    AST::FalseLiteral *falseLiteral = AST::cast<AST::FalseLiteral *>(expressionNode);
    if (falseLiteral)
        return false;

    AST::NumericLiteral *numericLiteral = AST::cast<AST::NumericLiteral *>(expressionNode);
    if (numericLiteral)
        return numericLiteral->value;

    return textAt(expressionNode->firstSourceLocation(), expressionNode->lastSourceLocation());
}

void SimpleAbstractStreamReader::setSourceLocation(const AST::SourceLocation &sourceLocation)
{
    m_currentSourceLocation = sourceLocation;
}

QString SimpleAbstractStreamReader::textAt(const AST::SourceLocation &from,
                                           const AST::SourceLocation &to)
{
    return m_source.mid(from.offset, to.end() - from.begin());
}

SimpleReader::SimpleReader()
{
}

SimpleReaderNode::Ptr SimpleReader::readFile(const QString &fileName)
{
    SimpleAbstractStreamReader::readFile(fileName);
    return m_rootNode;
}

SimpleReaderNode::Ptr SimpleReader::readFromSource(const QString &source)
{
    SimpleAbstractStreamReader::readFromSource(source);
    return m_rootNode;
}

void SimpleReader::elementStart(const QString &name)
{
    qCDebug(simpleReaderLog) << "elementStart()" << name;

    SimpleReaderNode::Ptr newNode = SimpleReaderNode::create(name, m_currentNode);

    if (newNode->isRoot())
        m_rootNode = newNode;

    Q_ASSERT(newNode->isValid());

    m_currentNode = newNode;
}

void SimpleReader::elementEnd()
{
    Q_ASSERT(m_currentNode);

    qCDebug(simpleReaderLog) << "elementEnd()" << m_currentNode.data()->name();

    m_currentNode = m_currentNode.data()->parent();
}

void SimpleReader::propertyDefinition(const QString &name, const QVariant &value)
{
    Q_ASSERT(m_currentNode);

    qCDebug(simpleReaderLog) << "propertyDefinition()" << m_currentNode.data()->name() << name << value;

    if (m_currentNode.data()->propertyNames().contains(name))
        addError(tr("Property is defined twice."), currentSourceLocation());

    m_currentNode.data()->setProperty(name, value);
}

} // namespace QmlJS
