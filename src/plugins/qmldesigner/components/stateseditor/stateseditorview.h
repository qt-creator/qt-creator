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
    QString currentStateName() const;

    // AbstractView
    void modelAttached(Model *model) QTC_OVERRIDE;
    void modelAboutToBeDetached(Model *model) QTC_OVERRIDE;
    void propertiesRemoved(const QList<AbstractProperty>& propertyList) QTC_OVERRIDE;
    void variantPropertiesChanged(const QList<VariantProperty>& propertyList,
                                  PropertyChangeFlags propertyChange) QTC_OVERRIDE;

    void nodeAboutToBeRemoved(const ModelNode &removedNode) QTC_OVERRIDE;
    void nodeRemoved(const ModelNode &removedNode,
                     const NodeAbstractProperty &parentProperty,
                     PropertyChangeFlags propertyChange) QTC_OVERRIDE;
    void nodeAboutToBeReparented(const ModelNode &node,
                                 const NodeAbstractProperty &newPropertyParent,
                                 const NodeAbstractProperty &oldPropertyParent,
                                 AbstractView::PropertyChangeFlags propertyChange) QTC_OVERRIDE;
    void nodeReparented(const ModelNode &node,
                        const NodeAbstractProperty &newPropertyParent,
                        const NodeAbstractProperty &oldPropertyParent,
                        AbstractView::PropertyChangeFlags propertyChange) QTC_OVERRIDE;
    void nodeOrderChanged(const NodeListProperty &listProperty, const ModelNode &movedNode, int oldIndex) QTC_OVERRIDE;


    // QmlModelView
    void actualStateChanged(const ModelNode &node) QTC_OVERRIDE;

    void scriptFunctionsChanged(const ModelNode &node, const QStringList &scriptFunctionList) QTC_OVERRIDE;
    void nodeIdChanged(const ModelNode &node, const QString &newId, const QString &oldId) QTC_OVERRIDE;
    void bindingPropertiesChanged(const QList<BindingProperty> &propertyList, PropertyChangeFlags propertyChange) QTC_OVERRIDE;
    void signalHandlerPropertiesChanged(const QVector<SignalHandlerProperty>& propertyList,PropertyChangeFlags propertyChange) QTC_OVERRIDE;
    void selectedNodesChanged(const QList<ModelNode> &selectedNodeList, const QList<ModelNode> &lastSelectedNodeList) QTC_OVERRIDE;

    void instancesPreviewImageChanged(const QVector<ModelNode> &nodeList) QTC_OVERRIDE;

    WidgetInfo widgetInfo() QTC_OVERRIDE;


public slots:
    void synchonizeCurrentStateFromWidget();
    void createNewState();
    void removeState(int nodeId);

private:
    StatesEditorWidget *statesEditorWidget() const;
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
