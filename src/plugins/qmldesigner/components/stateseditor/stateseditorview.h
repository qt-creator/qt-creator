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

#include <qmlstate.h>

namespace QmlDesigner {


class StatesEditorModel;
class StatesEditorWidget;

class StatesEditorView : public AbstractView {
    Q_OBJECT

public:
    explicit StatesEditorView(QObject *parent = 0);
    ~StatesEditorView();

    void renameState(int internalNodeId,const QString &newName);
    void setWhenCondition(int internalNodeId, const QString &condition);
    void resetWhenCondition(int internalNodeId);
    bool validStateName(const QString &name) const;
    QString currentStateName() const;
    void setCurrentState(const QmlModelState &state);
    QmlModelState baseState() const;
    QmlModelStateGroup rootStateGroup() const;

    // AbstractView
    void modelAttached(Model *model) override;
    void modelAboutToBeDetached(Model *model) override;
    void propertiesRemoved(const QList<AbstractProperty>& propertyList) override;
    void nodeAboutToBeRemoved(const ModelNode &removedNode) override;
    void nodeRemoved(const ModelNode &removedNode,
                     const NodeAbstractProperty &parentProperty,
                     PropertyChangeFlags propertyChange) override;
    void nodeAboutToBeReparented(const ModelNode &node,
                                 const NodeAbstractProperty &newPropertyParent,
                                 const NodeAbstractProperty &oldPropertyParent,
                                 AbstractView::PropertyChangeFlags propertyChange) override;
    void nodeReparented(const ModelNode &node,
                        const NodeAbstractProperty &newPropertyParent,
                        const NodeAbstractProperty &oldPropertyParent,
                        AbstractView::PropertyChangeFlags propertyChange) override;
    void nodeOrderChanged(const NodeListProperty &listProperty, const ModelNode &movedNode, int oldIndex) override;
    void bindingPropertiesChanged(const QList<BindingProperty>& propertyList, PropertyChangeFlags propertyChange) override;


    // AbstractView
    void currentStateChanged(const ModelNode &node) override;

    void instancesPreviewImageChanged(const QVector<ModelNode> &nodeList) override;

    WidgetInfo widgetInfo() override;

    void rootNodeTypeChanged(const QString &type, int majorVersion, int minorVersion) override;

    void toggleStatesViewExpanded();

public slots:
    void synchonizeCurrentStateFromWidget();
    void createNewState();
    void removeState(int nodeId);

private:
    StatesEditorWidget *statesEditorWidget() const;
    void resetModel();
    void addState();
    void duplicateCurrentState();
    void checkForWindow();

private:
    QPointer<StatesEditorModel> m_statesEditorModel;
    QPointer<StatesEditorWidget> m_statesEditorWidget;
    int m_lastIndex;
    bool m_block = false;
};

} // namespace QmlDesigner
