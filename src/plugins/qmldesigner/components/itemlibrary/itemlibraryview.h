/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef ITEMLIBRARYVIEW_H
#define ITEMLIBRARYVIEW_H

#include <abstractview.h>

#include <QWeakPointer>


namespace QmlDesigner {

class ItemLibraryWidget;


class ItemLibraryView : public AbstractView
{
    Q_OBJECT

public:
    ItemLibraryView(QObject* parent = 0);
    ~ItemLibraryView();

    ItemLibraryWidget *widget();

    // AbstractView
    void modelAttached(Model *model);
    void modelAboutToBeDetached(Model *model);

    void importAdded(const Import &import);
    void importRemoved(const Import &import);

    void nodeCreated(const ModelNode &createdNode);
    void nodeRemoved(const ModelNode &removedNode, const NodeAbstractProperty &parentProperty, PropertyChangeFlags propertyChange);
    void propertiesRemoved(const QList<AbstractProperty> &propertyList);
    void variantPropertiesChanged(const QList<VariantProperty> &propertyList, PropertyChangeFlags propertyChange);
    void bindingPropertiesChanged(const QList<BindingProperty> &propertyList, PropertyChangeFlags propertyChange);

    void nodeAboutToBeRemoved(const ModelNode &removedNode);
    void nodeOrderChanged(const NodeListProperty &listProperty, const ModelNode &movedNode, int oldIndex);

    void nodeAboutToBeReparented(const ModelNode &node, const NodeAbstractProperty &newPropertyParent, const NodeAbstractProperty &oldPropertyParent, AbstractView::PropertyChangeFlags propertyChange);
    void nodeReparented(const ModelNode &node, const NodeAbstractProperty &newPropertyParent, const NodeAbstractProperty &oldPropertyParent, AbstractView::PropertyChangeFlags propertyChange);
    void rootNodeTypeChanged(const QString &type, int majorVersion, int minorVersion);
    void nodeIdChanged(const ModelNode& node, const QString& newId, const QString& oldId);
    void propertiesAboutToBeRemoved(const QList<AbstractProperty>& propertyList);

    void selectedNodesChanged(const QList<ModelNode> &selectedNodeList,
                                      const QList<ModelNode> &lastSelectedNodeList);
    void auxiliaryDataChanged(const ModelNode &node, const QString &name, const QVariant &data);
    void scriptFunctionsChanged(const ModelNode &node, const QStringList &scriptFunctionList);
    void instancePropertyChange(const QList<QPair<ModelNode, QString> > &propertyList);
    void instancesCompleted(const QVector<ModelNode> &completedNodeList);

protected:
    void updateImports();

private:
    QWeakPointer<ItemLibraryWidget> m_widget;
};

}
#endif // ITEMLIBRARYVIEW_H
