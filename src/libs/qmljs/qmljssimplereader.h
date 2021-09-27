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

#pragma once

#include <qmljs/qmljs_global.h>

#ifdef QT_CREATOR
#include <qmljs/parser/qmljsastfwd_p.h>
#else
#include <parser/qqmljsastfwd_p.h>
#endif

#include <QHash>
#include <QList>
#include <QSharedPointer>
#include <QStringList>
#include <QVariant>
#include <QWeakPointer>

// for Q_DECLARE_TR_FUNCTIONS
#include <QCoreApplication>

namespace QmlJS {

#ifndef QT_CREATOR
using SourceLocation = QQmlJS::SourceLocation;
#endif

class QMLJS_EXPORT SimpleReaderNode
{
public:
    struct Property
    {
        QVariant value;
        SourceLocation nameLocation;
        SourceLocation valueLocation;

        bool isValid() const { return !value.isNull() && value.isValid(); }
        bool isDefaultValue() const
        {
            return !value.isNull() && !nameLocation.isValid() && !valueLocation.isValid();
        }
    };
    typedef QSharedPointer<SimpleReaderNode> Ptr;
    typedef QWeakPointer<SimpleReaderNode> WeakPtr;
    typedef QHash<QString, Property> PropertyHash;
    typedef QList<Ptr> List;

    Property property(const QString &name) const;
    QStringList propertyNames() const;
    PropertyHash properties() const;
    bool isRoot() const;
    bool isValid() const;
    static Ptr invalidNode();
    WeakPtr parent() const;
    QString name() const;
    SourceLocation nameLocation() const;
    const List children() const;

protected:
    SimpleReaderNode();
    SimpleReaderNode(const QString &name, const SourceLocation &nameLocation, WeakPtr parent);
    static Ptr create(const QString &name, const SourceLocation &nameLocation, WeakPtr parent);
    void setProperty(const QString &name,
                     const SourceLocation &nameLocation,
                     const QVariant &value,
                     const SourceLocation &valueLocation);

private:
    const QString m_name;
    const SourceLocation m_nameLocation;
    PropertyHash m_properties;
    const WeakPtr m_parentNode;
    List m_children;
    WeakPtr m_weakThis;

    friend class SimpleReader;
};

#ifndef QT_CREATOR
using namespace QQmlJS;
#endif

class QMLJS_EXPORT SimpleAbstractStreamReader
{
    Q_DECLARE_TR_FUNCTIONS(QmlJS::SimpleAbstractStreamReader)

public:
    SimpleAbstractStreamReader();
    virtual ~SimpleAbstractStreamReader();
    bool readFile(const QString &fileName);
    bool readFromSource(const QString &source);
    QStringList errors() const;

protected:
    void addError(const QString &error, const SourceLocation &sourceLocation = SourceLocation());
    SourceLocation currentSourceLocation() const;

    virtual void elementStart(const QString &name, const SourceLocation &nameLocation) = 0;
    virtual void elementEnd() = 0;
    virtual void propertyDefinition(const QString &name,
                                    const SourceLocation &nameLocation,
                                    const QVariant &value,
                                    const SourceLocation &valueLocation)
        = 0;

private:
    bool readDocument(AST::UiProgram *ast);
    void readChildren(AST::UiObjectDefinition *ast);
    void readChild(AST::UiObjectDefinition *uiObjectDefinition);
    void readProperties(AST::UiObjectDefinition *ast);
    void readProperty(AST::UiScriptBinding *uiScriptBinding);
    std::pair<QVariant, SourceLocation> parsePropertyScriptBinding(
        AST::UiScriptBinding *ExpressionNode);
    QVariant parsePropertyExpression(AST::ExpressionNode *expressionNode);
    void setSourceLocation(const SourceLocation &sourceLocation);
    QString textAt(const SourceLocation &from, const SourceLocation &to);

    QStringList m_errors;
    SourceLocation m_currentSourceLocation;
    QString m_source;
};

class QMLJS_EXPORT SimpleReader: public SimpleAbstractStreamReader
{
    Q_DECLARE_TR_FUNCTIONS(QmlJS::SimpleReader)

public:
    SimpleReader();
    SimpleReaderNode::Ptr readFile(const QString &fileName);
    SimpleReaderNode::Ptr readFromSource(const QString &source);

protected:
    void elementStart(const QString &name, const SourceLocation &nameLocation) override;
    void elementEnd() override;
    void propertyDefinition(const QString &name,
                            const SourceLocation &nameLocation,
                            const QVariant &value,
                            const SourceLocation &valueLocation) override;

private:
    SimpleReaderNode::Ptr m_rootNode;
    SimpleReaderNode::WeakPtr m_currentNode;
};

}  // namespace QmlJS
