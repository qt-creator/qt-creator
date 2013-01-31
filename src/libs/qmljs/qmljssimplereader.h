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

#ifndef QMLJS_SIMPLEREADER_H
#define QMLJS_SIMPLEREADER_H

#include <qmljs/qmljs_global.h>
#include <qmljs/parser/qmljsastfwd_p.h>

#include <QHash>
#include <QList>
#include <QStringList>
#include <QSharedPointer>
#include <QWeakPointer>

// for Q_DECLARE_TR_FUNCTIONS
#include <QCoreApplication>

namespace QmlJS {

class QMLJS_EXPORT SimpleReaderNode
{
public:
    typedef QSharedPointer<SimpleReaderNode> Ptr;
    typedef QWeakPointer<SimpleReaderNode> WeakPtr;
    typedef QHash<QString, QVariant> PropertyHash;
    typedef QList<Ptr> List;

    QVariant property(const QString &name) const;
    QStringList propertyNames() const;
    PropertyHash properties() const;
    bool isRoot() const;
    bool isValid() const;
    static Ptr invalidNode();
    WeakPtr parent() const;
    QString name() const;
    const List children() const;

protected:
    SimpleReaderNode();
    SimpleReaderNode(const QString &name, WeakPtr parent);
    static Ptr create(const QString &name, WeakPtr parent);
    void setProperty(const QString &name, const QVariant &value);

private:
    const QString m_name;
    PropertyHash m_properties;
    const WeakPtr m_parentNode;
    List m_children;
    WeakPtr m_weakThis;

    friend class SimpleReader;
};

class QMLJS_EXPORT SimpleAbstractStreamReader
{
    Q_DECLARE_TR_FUNCTIONS(QmlJS::SimpleAbstractStreamReader)

public:
    SimpleAbstractStreamReader();
    bool readFile(const QString &fileName);
    bool readFromSource(const QString &source);
    QStringList errors() const;

protected:
    void addError(const QString &error, const AST::SourceLocation &sourceLocation = AST::SourceLocation());
    AST::SourceLocation currentSourceLocation() const;

    virtual void elementStart(const QString &name) = 0;
    virtual void elementEnd() = 0;
    virtual void propertyDefinition(const QString &name, const QVariant &value) = 0;

private:
    bool readDocument(AST::UiProgram *ast);
    void readChildren(AST::UiObjectDefinition *ast);
    void readChild(AST::UiObjectDefinition *uiObjectDefinition);
    void readProperties(AST::UiObjectDefinition *ast);
    void readProperty(AST::UiScriptBinding *uiScriptBinding);
    QVariant parsePropertyScriptBinding(AST::UiScriptBinding *ExpressionNode);
    QVariant parsePropertyExpression(AST::ExpressionNode *expressionNode);
    void setSourceLocation(const AST::SourceLocation &sourceLocation);

    QStringList m_errors;
    AST::SourceLocation m_currentSourceLocation;
};

class QMLJS_EXPORT SimpleReader: public SimpleAbstractStreamReader
{
    Q_DECLARE_TR_FUNCTIONS(QmlJS::SimpleReader)

public:
    SimpleReader();
    SimpleReaderNode::Ptr readFile(const QString &fileName);
    SimpleReaderNode::Ptr readFromSource(const QString &source);

protected:
    virtual void elementStart(const QString &name);
    virtual void elementEnd();
    virtual void propertyDefinition(const QString &name, const QVariant &value);

private:
    SimpleReaderNode::Ptr m_rootNode;
    SimpleReaderNode::WeakPtr m_currentNode;
};

}  // namespace QmlJS

#endif // QMLJS_SIMPLEREADER_H
