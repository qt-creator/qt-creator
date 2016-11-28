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

#include <abstractview.h>
#include <nodeinstanceview.h>
#include <QVariant>
#include <QStringList>
#include <model.h>
#include <qmlobjectnode.h>
#include <qmlitemnode.h>

class TestView : public QmlDesigner::AbstractView
{
    Q_OBJECT
public:
    struct MethodCall {
        MethodCall(const QString &n, const QStringList &args) :
                name(n), arguments(args)
        {
        }

        QString name;
        QStringList arguments;
    };

    TestView(QmlDesigner::Model *model);

    void modelAttached(QmlDesigner::Model *model);
    void modelAboutToBeDetached(QmlDesigner::Model *model);
    void nodeCreated(const QmlDesigner::ModelNode &createdNode);
    void nodeAboutToBeRemoved(const QmlDesigner::ModelNode &removedNode);
    void nodeRemoved(const QmlDesigner::ModelNode &removedNode, const QmlDesigner::NodeAbstractProperty &parentProperty, AbstractView::PropertyChangeFlags propertyChange);
    void nodeReparented(const QmlDesigner::ModelNode &node, const QmlDesigner::NodeAbstractProperty &newPropertyParent, const QmlDesigner::NodeAbstractProperty &oldPropertyParent, AbstractView::PropertyChangeFlags propertyChange);
    void nodeAboutToBeReparented(const QmlDesigner::ModelNode &node, const QmlDesigner::NodeAbstractProperty &newPropertyParent, const QmlDesigner::NodeAbstractProperty &oldPropertyParent, AbstractView::PropertyChangeFlags propertyChange);
    void nodeIdChanged(const QmlDesigner::ModelNode& node, const QString& newId, const QString& oldId);
    void rootNodeTypeChanged(const QString &type, int majorVersion, int minorVersion);

    void bindingPropertiesChanged(const QList<QmlDesigner::BindingProperty>& propertyList, PropertyChangeFlags propertyChange);
    void variantPropertiesChanged(const QList<QmlDesigner::VariantProperty>& propertyList, PropertyChangeFlags propertyChange);
    void propertiesAboutToBeRemoved(const QList<QmlDesigner::AbstractProperty> &propertyList);
    void propertiesRemoved(const QList<QmlDesigner::AbstractProperty>& propertyList);
    void signalHandlerPropertiesChanged(const QVector<QmlDesigner::SignalHandlerProperty>& propertyList, PropertyChangeFlags propertyChange);

    void importsChanged(const QList<QmlDesigner::Import> &addedImports, const QList<QmlDesigner::Import> &removedImports);



    void fileUrlChanged(const QUrl &oldBaseUrl, const QUrl &newBaseUrl);
    void selectedNodesChanged(const QList<QmlDesigner::ModelNode> &selectedNodeList,
                              const QList<QmlDesigner::ModelNode> &lastSelectedNodeList);

    void nodeOrderChanged(const QmlDesigner::NodeListProperty &listProperty, const QmlDesigner::ModelNode &movedNode, int oldIndex);


    virtual void instancePropertyChanged(const QList<QPair<QmlDesigner::ModelNode, QmlDesigner::PropertyName> > &propertyList);
    virtual void instancesCompleted(const QVector<QmlDesigner::ModelNode> &completedNodeList);
    virtual void instanceInformationsChanged(const QMultiHash<QmlDesigner::ModelNode, QmlDesigner::InformationName> &informationChangeHash);
    virtual void instancesRenderImageChanged(const QVector<QmlDesigner::ModelNode> &nodeList);
    virtual void instancesPreviewImageChanged(const QVector<QmlDesigner::ModelNode> &nodeList);
    virtual void instancesChildrenChanged(const QVector<QmlDesigner::ModelNode> &nodeList);
    virtual void instancesToken(const QString &tokenName, int tokenNumber, const QVector<QmlDesigner::ModelNode> &nodeVector);

    virtual void nodeSourceChanged(const QmlDesigner::ModelNode &modelNode, const QString &newNodeSource);

    virtual void rewriterBeginTransaction() {}
    virtual void rewriterEndTransaction() {}

    void scriptFunctionsChanged(const QmlDesigner::ModelNode &node, const QStringList &scriptFunctionList);

    void currentStateChanged(const QmlDesigner::ModelNode &node);
    QList<MethodCall> &methodCalls();

    QString lastFunction() const;

    QmlDesigner::NodeInstanceView *nodeInstanceView() const;

    QmlDesigner::QmlObjectNode rootQmlObjectNode() const;

    void setCurrentState(const QmlDesigner::QmlModelState &node);

    QmlDesigner::QmlItemNode rootQmlItemNode() const;

    QmlDesigner::QmlModelState baseState() const;

    QmlDesigner::QmlObjectNode createQmlObjectNode(const QmlDesigner::TypeName &typeName,
                         int majorVersion,
                         int minorVersion,
                         const QmlDesigner::PropertyListType &propertyList = QmlDesigner::PropertyListType());

private:
    QList<MethodCall> m_methodCalls;
    static QString serialize(AbstractView::PropertyChangeFlags change);
};

bool operator==(TestView::MethodCall call1, TestView::MethodCall call2);
QDebug operator<<(QDebug debug, TestView::MethodCall call);
