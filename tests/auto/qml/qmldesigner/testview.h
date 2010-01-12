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

#ifndef TESTVIEW_H
#define TESTVIEW_H

#include <modelnode.h>
#include <qmlmodelview.h>
#include <QVariant>
#include <QStringList>

class TestView : public QmlDesigner::QmlModelView
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

    TestView(QObject *parent = 0);

    void modelAttached(QmlDesigner::Model *model);
    void modelAboutToBeDetached(QmlDesigner::Model *model);
    void nodeCreated(const QmlDesigner::ModelNode &createdNode);
    void nodeAboutToBeRemoved(const QmlDesigner::ModelNode &removedNode);
    void nodeRemoved(const QmlDesigner::ModelNode &removedNode, const QmlDesigner::NodeAbstractProperty &parentProperty, AbstractView::PropertyChangeFlags propertyChange);
    void nodeReparented(const QmlDesigner::ModelNode &node, const QmlDesigner::NodeAbstractProperty &newPropertyParent, const QmlDesigner::NodeAbstractProperty &oldPropertyParent, AbstractView::PropertyChangeFlags propertyChange);
    void nodeIdChanged(const QmlDesigner::ModelNode& node, const QString& newId, const QString& oldId);
    void nodeTypeChanged(const QmlDesigner::ModelNode &node,const QString &type, int majorVersion, int minorVersion);

    void bindingPropertiesChanged(const QList<QmlDesigner::BindingProperty>& propertyList, PropertyChangeFlags propertyChange);
    void variantPropertiesChanged(const QList<QmlDesigner::VariantProperty>& propertyList, PropertyChangeFlags propertyChange);
    void propertiesAboutToBeRemoved(const QList<QmlDesigner::AbstractProperty> &propertyList);



    void fileUrlChanged(const QUrl &oldBaseUrl, const QUrl &newBaseUrl);
    void selectedNodesChanged(const QList<QmlDesigner::ModelNode> &selectedNodeList,
                              const QList<QmlDesigner::ModelNode> &lastSelectedNodeList);

    void nodeSlidedToIndex(const QmlDesigner::NodeListProperty &listProperty, int newIndex, int oldIndex);

    void stateChanged(const QmlDesigner::QmlModelState &newQmlModelState, const QmlDesigner::QmlModelState &oldQmlModelState);

    QList<MethodCall> &methodCalls();

    QString lastFunction() const;

    QmlDesigner::NodeInstanceView *nodeInstanceView() const;

    QmlDesigner::NodeInstance instanceForModelNode(const QmlDesigner::ModelNode &modelNode);

private:
    QList<MethodCall> m_methodCalls;
    static QString serialize(AbstractView::PropertyChangeFlags change);
};

bool operator==(TestView::MethodCall call1, TestView::MethodCall call2);
QDebug operator<<(QDebug debug, TestView::MethodCall call);

#endif // TESTVIEW_H
