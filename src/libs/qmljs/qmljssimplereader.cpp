// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmljssimplereader.h"

#ifdef QT_CREATOR
#include "parser/qmljsengine_p.h"
#include "parser/qmljslexer_p.h"
#include "parser/qmljsparser_p.h"
#else
#include "parser/qqmljsengine_p.h"
#include "parser/qqmljslexer_p.h"
#include "qqmljsparser_p.h"
#endif

#include "qmljstr.h"
#include "qmljsutils.h"

#include <QFile>
#include <QLoggingCategory>
#include <QVariant>

static Q_LOGGING_CATEGORY(simpleReaderLog, "qtc.qmljs.simpleReader", QtWarningMsg)

    namespace QmlJS
{
#ifdef QT_CREATOR
    using UiQualifiedId = QmlJS::AST::UiQualifiedId;
#else
    using UiQualifiedId = QQmlJS::AST::UiQualifiedId;
#endif

    static SourceLocation toSourceLocation(SourceLocation first, SourceLocation last)
    {
        first.length = last.end() - first.begin();
        return first;
    }

    static SourceLocation toSourceLocation(UiQualifiedId * qualifiedId)
    {
        SourceLocation first = qualifiedId->firstSourceLocation();
        SourceLocation last;
        for (UiQualifiedId *iter = qualifiedId; iter; iter = iter->next) {
            if (iter->lastSourceLocation().isValid())
                last = iter->lastSourceLocation();
        }
        return toSourceLocation(first, last);
    }

    SimpleReaderNode::Property SimpleReaderNode::property(const QString &name) const
    {
        return m_properties.value(name);
    }

    QStringList SimpleReaderNode::propertyNames() const { return m_properties.keys(); }

    SimpleReaderNode::PropertyHash SimpleReaderNode::properties() const { return m_properties; }

    bool SimpleReaderNode::isRoot() const { return m_parentNode.isNull(); }

    bool SimpleReaderNode::isValid() const { return !m_name.isEmpty(); }

    SimpleReaderNode::Ptr SimpleReaderNode::invalidNode() { return Ptr(new SimpleReaderNode); }

    SimpleReaderNode::WeakPtr SimpleReaderNode::parent() const { return m_parentNode; }

    QString SimpleReaderNode::name() const { return m_name; }

    SourceLocation SimpleReaderNode::nameLocation() const { return m_nameLocation; }

    SimpleReaderNode::SimpleReaderNode() {}

    SimpleReaderNode::SimpleReaderNode(const QString &name,
                                       const SourceLocation &nameLocation,
                                       WeakPtr parent)
        : m_name(name)
        , m_nameLocation(nameLocation)
        , m_parentNode(parent)
    {}

    SimpleReaderNode::Ptr SimpleReaderNode::create(const QString &name,
                                                   const SourceLocation &nameLocation,
                                                   WeakPtr parent)
    {
        Ptr newNode(new SimpleReaderNode(name, nameLocation, parent));
        newNode->m_weakThis = newNode;
        if (parent)
            parent.toStrongRef().data()->m_children.append(newNode);
        return newNode;
    }

    const SimpleReaderNode::List &SimpleReaderNode::children() const
    {
        return m_children;
    }

    void SimpleReaderNode::setProperty(const QString &name,
                                       const SourceLocation &nameLocation,
                                       const QVariant &value,
                                       const SourceLocation &valueLocation)
    {
        m_properties.insert(name, {value, nameLocation, valueLocation});
    }

    SimpleAbstractStreamReader::SimpleAbstractStreamReader() {}

    SimpleAbstractStreamReader::~SimpleAbstractStreamReader() {}

    bool SimpleAbstractStreamReader::readFile(const QString &fileName)
    {
        QFile file(fileName);
        if (file.open(QIODevice::ReadOnly)) {
            QByteArray source = file.readAll();
            file.close();
            return readFromSource(QString::fromLocal8Bit(source));
        }
        addError(Tr::tr("Cannot find file %1.").arg(fileName));
        return false;
    }

    bool SimpleAbstractStreamReader::readFromSource(const QString &source)
    {
        m_errors.clear();
        m_currentSourceLocation = SourceLocation();

        m_source = source;

        Engine engine;
        Lexer lexer(&engine);
        Parser parser(&engine);

        lexer.setCode(source, /*line = */ 1, /*qmlMode = */ true);

        if (!parser.parse()) {
            QString errorMessage = QString::fromLatin1("%1:%2: %3")
                                       .arg(QString::number(parser.errorLineNumber()),
                                            QString::number(parser.errorColumnNumber()),
                                            parser.errorMessage());
            addError(errorMessage);
            return false;
        }
        return readDocument(parser.ast());
    }

    QStringList SimpleAbstractStreamReader::errors() const { return m_errors; }

    void SimpleAbstractStreamReader::addError(const QString &error,
                                              const SourceLocation &sourceLocation)
    {
        m_errors << QString::fromLatin1("%1:%2: %3\n")
                        .arg(QString::number(sourceLocation.startLine),
                             QString::number(sourceLocation.startColumn),
                             error);
    }

    SourceLocation SimpleAbstractStreamReader::currentSourceLocation() const
    {
        return m_currentSourceLocation;
    }

    bool SimpleAbstractStreamReader::readDocument(AST::UiProgram * ast)
    {
        if (!ast) {
            addError(Tr::tr("Could not parse document."));
            return false;
        }

        AST::UiObjectDefinition *uiObjectDefinition = AST::cast<AST::UiObjectDefinition *>(
            ast->members->member);
        if (!uiObjectDefinition) {
            addError(Tr::tr("Expected document to contain a single object definition."));
            return false;
        }
        readChild(uiObjectDefinition);

        m_source.clear();

        return errors().isEmpty();
    }

    void SimpleAbstractStreamReader::readChildren(AST::UiObjectDefinition * uiObjectDefinition)
    {
        Q_ASSERT(uiObjectDefinition);

        for (AST::UiObjectMemberList *it = uiObjectDefinition->initializer->members; it;
             it = it->next) {
            AST::UiObjectMember *member = it->member;
            AST::UiObjectDefinition *uiObjectDefinition = AST::cast<AST::UiObjectDefinition *>(
                member);
            if (uiObjectDefinition)
                readChild(uiObjectDefinition);
        }
    }

    void SimpleAbstractStreamReader::readChild(AST::UiObjectDefinition * uiObjectDefinition)
    {
        Q_ASSERT(uiObjectDefinition);

        setSourceLocation(uiObjectDefinition->firstSourceLocation());

        elementStart(toString(uiObjectDefinition->qualifiedTypeNameId),
                     toSourceLocation(uiObjectDefinition->qualifiedTypeNameId));

        readProperties(uiObjectDefinition);
        readChildren(uiObjectDefinition);

        elementEnd();
    }

    void SimpleAbstractStreamReader::readProperties(AST::UiObjectDefinition * uiObjectDefinition)
    {
        Q_ASSERT(uiObjectDefinition);

        for (AST::UiObjectMemberList *it = uiObjectDefinition->initializer->members; it;
             it = it->next) {
            AST::UiObjectMember *member = it->member;
            AST::UiScriptBinding *scriptBinding = AST::cast<AST::UiScriptBinding *>(member);
            if (scriptBinding)
                readProperty(scriptBinding);
        }
    }

    void SimpleAbstractStreamReader::readProperty(AST::UiScriptBinding * uiScriptBinding)
    {
        Q_ASSERT(uiScriptBinding);

        setSourceLocation(uiScriptBinding->firstSourceLocation());

        const QString name = toString(uiScriptBinding->qualifiedId);
        auto nameLoc = toSourceLocation(uiScriptBinding->qualifiedId);
        auto value = parsePropertyScriptBinding(uiScriptBinding);

        propertyDefinition(name, nameLoc, value.first, value.second);
    }

    std::pair<QVariant, SourceLocation> SimpleAbstractStreamReader::parsePropertyScriptBinding(
        AST::UiScriptBinding * uiScriptBinding)
    {
        Q_ASSERT(uiScriptBinding);

        AST::ExpressionStatement *expStmt = AST::cast<AST::ExpressionStatement *>(
            uiScriptBinding->statement);
        if (!expStmt) {
            addError(Tr::tr("Expected expression statement after colon."),
                     uiScriptBinding->statement->firstSourceLocation());
            return std::make_pair(QVariant(), SourceLocation());
        }

        return std::make_pair(parsePropertyExpression(expStmt->expression),
                              toSourceLocation(expStmt->firstSourceLocation(),
                                               expStmt->lastSourceLocation()));
    }

    QVariant SimpleAbstractStreamReader::parsePropertyExpression(AST::ExpressionNode
                                                                 * expressionNode)
    {
        Q_ASSERT(expressionNode);

        AST::ArrayPattern *arrayLiteral = AST::cast<AST::ArrayPattern *>(expressionNode);

        if (arrayLiteral) {
            QList<QVariant> variantList;
            for (AST::PatternElementList *it = arrayLiteral->elements; it; it = it->next)
                variantList << parsePropertyExpression(it->element->initializer);
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

    void SimpleAbstractStreamReader::setSourceLocation(const SourceLocation &sourceLocation)
    {
        m_currentSourceLocation = sourceLocation;
    }

    QString SimpleAbstractStreamReader::textAt(const SourceLocation &from, const SourceLocation &to)
    {
        return m_source.mid(from.offset, to.end() - from.begin());
    }

    SimpleReader::SimpleReader() {}

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

    void SimpleReader::elementStart(const QString &name, const SourceLocation &nameLocation)
    {
        qCDebug(simpleReaderLog) << "elementStart()" << name;

        SimpleReaderNode::Ptr newNode = SimpleReaderNode::create(name, nameLocation, m_currentNode);

        if (newNode->isRoot())
            m_rootNode = newNode;

        Q_ASSERT(newNode->isValid());

        m_currentNode = newNode;
    }

    void SimpleReader::elementEnd()
    {
        Q_ASSERT(m_currentNode);

        qCDebug(simpleReaderLog) << "elementEnd()" << m_currentNode.toStrongRef().data()->name();

        m_currentNode = m_currentNode.toStrongRef().data()->parent();
    }

    void SimpleReader::propertyDefinition(const QString &name,
                                          const SourceLocation &nameLocation,
                                          const QVariant &value,
                                          const SourceLocation &valueLocation)
    {
        Q_ASSERT(m_currentNode);

        qCDebug(simpleReaderLog) << "propertyDefinition()"
                                 << m_currentNode.toStrongRef().data()->name() << name << value;

        if (m_currentNode.toStrongRef().data()->propertyNames().contains(name)) {
            auto previousSourceLoc = m_currentNode.toStrongRef().data()->property(name).nameLocation;
            addError(Tr::tr("Property is defined twice, previous definition at %1:%2")
                         .arg(QString::number(previousSourceLoc.startLine),
                              QString::number(previousSourceLoc.startColumn)),
                     currentSourceLocation());
        }

        m_currentNode.toStrongRef().data()->setProperty(name, nameLocation, value, valueLocation);
    }

} // namespace QmlJS
