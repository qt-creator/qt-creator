/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef STATESEDITORVIEW_H
#define STATESEDITORVIEW_H

#include <qmlmodelview.h>

namespace QmlDesigner {


class StatesEditorModel;
class StatesEditorWidget;

class StatesEditorView : public QmlModelView {
    Q_OBJECT

public:
    explicit StatesEditorView(QObject *parent = 0);

    void renameState(int nodeId,const QString &newName);
    bool validStateName(const QString &name) const;

    void nodeInstancePropertyChanged(const ModelNode &node, const QString &propertyName);

    // AbstractView
    void modelAttached(Model *model);
    void modelAboutToBeDetached(Model *model);
    void propertiesAboutToBeRemoved(const QList<AbstractProperty>& propertyList);
    void variantPropertiesChanged(const QList<VariantProperty>& propertyList, PropertyChangeFlags propertyChange);

    void nodeAboutToBeRemoved(const ModelNode &removedNode);
    void nodeRemoved(const ModelNode &removedNode, const NodeAbstractProperty &parentProperty, PropertyChangeFlags propertyChange);
    void nodeAboutToBeReparented(const ModelNode &node, const NodeAbstractProperty &newPropertyParent, const NodeAbstractProperty &oldPropertyParent, AbstractView::PropertyChangeFlags propertyChange);
    void nodeReparented(const ModelNode &node, const NodeAbstractProperty &newPropertyParent, const NodeAbstractProperty &oldPropertyParent, AbstractView::PropertyChangeFlags propertyChange);
    void nodeOrderChanged(const NodeListProperty &listProperty, const ModelNode &movedNode, int oldIndex);


    // QmlModelView
    void stateChanged(const QmlModelState &newQmlModelState, const QmlModelState &oldQmlModelState);
    void transformChanged(const QmlObjectNode &qmlObjectNode, const QString &propertyName);
    void parentChanged(const QmlObjectNode &qmlObjectNode);
    void otherPropertyChanged(const QmlObjectNode &qmlObjectNode, const QString &propertyName);

    void customNotification(const AbstractView *view, const QString &identifier, const QList<ModelNode> &nodeList, const QList<QVariant> &data);
    void scriptFunctionsChanged(const ModelNode &node, const QStringList &scriptFunctionList);
    void nodeIdChanged(const ModelNode &node, const QString &newId, const QString &oldId);
    void bindingPropertiesChanged(const QList<BindingProperty> &propertyList, PropertyChangeFlags propertyChange);
    void selectedNodesChanged(const QList<ModelNode> &selectedNodeList, const QList<ModelNode> &lastSelectedNodeList);

    StatesEditorWidget *widget();

public slots:
    void synchonizeCurrentStateFromWidget();
    void createNewState();
    void removeState(int nodeId);

private:
    void resetModel();
    void addState();
    void duplicateCurrentState();

private:
    QWeakPointer<StatesEditorModel> m_statesEditorModel;
    QWeakPointer<StatesEditorWidget> m_statesEditorWidget;
    int m_lastIndex;
};

} // namespace QmlDesigner

#endif // STATESEDITORVIEW_H
