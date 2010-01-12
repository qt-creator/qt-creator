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

#ifndef COMPONENTVIEW_H
#define COMPONENTVIEW_H

#include <abstractview.h>
#include <modelnode.h>

#include <QStringList>

class QStandardItemModel;

namespace QmlDesigner {

class ComponentView : public AbstractView
{
    Q_OBJECT

public:
    enum UserRoles
    {
        ModelNodeRole = Qt::UserRole
    };

    ComponentView(QObject *parent);

    void modelAttached(Model *model);
    void modelAboutToBeDetached(Model *model);

    void nodeCreated(const ModelNode &createdNode);
    void nodeAboutToBeRemoved(const ModelNode &removedNode);
    void nodeRemoved(const ModelNode &removedNode, const NodeAbstractProperty &parentProperty, PropertyChangeFlags propertyChange);
    void nodeReparented(const ModelNode &node, const NodeAbstractProperty &newPropertyParent, const NodeAbstractProperty &oldPropertyParent, AbstractView::PropertyChangeFlags propertyChange);
    void nodeIdChanged(const ModelNode& node, const QString& newId, const QString& oldId);
    void propertiesAboutToBeRemoved(const QList<AbstractProperty>& propertyList);
    void propertiesRemoved(const QList<AbstractProperty>& propertyList);
    void variantPropertiesChanged(const QList<VariantProperty>& propertyList, PropertyChangeFlags propertyChange);
    void bindingPropertiesChanged(const QList<BindingProperty>& propertyList, PropertyChangeFlags propertyChange);
    void nodeTypeChanged(const ModelNode &node,const QString &type, int majorVersion, int minorVersion);



    void selectedNodesChanged(const QList<ModelNode> &selectedNodeList,
                                      const QList<ModelNode> &lastSelectedNodeList);

    void fileUrlChanged(const QUrl &oldUrl, const QUrl &newUrl);

    void nodeOrderChanged(const NodeListProperty &listProperty, const ModelNode &movedNode, int oldIndex);

    void importsChanged();

    void auxiliaryDataChanged(const ModelNode &node, const QString &name, const QVariant &data);

    void customNotification(const AbstractView *view, const QString &identifier, const QList<ModelNode> &nodeList, const QList<QVariant> &data);


    QStandardItemModel *standardItemModel() const;

    ModelNode modelNode(int index) const;

signals:
    void componentListChanged(const QStringList &componentList);

private: //functions
    void updateModel();
    void searchForComponentAndAddToList(const ModelNode &node);
//    void searchForComponentAndRemoveFromList(const ModelNode &node);
    void appendWholeDocumentAsComponent();

private:
    QStringList m_componentList;
    QStandardItemModel *m_standardItemModel;
    bool m_listChanged;
};

} // namespace QmlDesigner

#endif // COMPONENTVIEW_H
