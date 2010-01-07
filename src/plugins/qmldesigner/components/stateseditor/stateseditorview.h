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

#ifndef STATESEDITORVIEW_H
#define STATESEDITORVIEW_H

#include <qmlmodelview.h>

namespace QmlDesigner {

namespace Internal {

class StatesEditorModel;

class StatesEditorView : public QmlModelView {
    Q_OBJECT

public:
    StatesEditorView(StatesEditorModel *model, QObject *parent = 0);

    void setCurrentState(int index);
    void setCurrentStateSilent(int index);
    void setBackCurrentState(int index, const QmlModelState &oldState);
    void createState(const QString &name);
    void removeState(int index);
    void renameState(int index,const QString &newName);
    void duplicateCurrentState(int index);

    QPixmap renderState(int i);
    QmlItemNode stateRootNode() { return m_stateRootNode; }

    // AbstractView
    void modelAttached(Model *model);
    void modelAboutToBeDetached(Model *model);
    void propertiesAboutToBeRemoved(const QList<AbstractProperty>& propertyList);
    void propertiesRemoved(const QList<AbstractProperty>& propertyList);
    void variantPropertiesChanged(const QList<VariantProperty>& propertyList, PropertyChangeFlags propertyChange);

    void nodeAboutToBeRemoved(const ModelNode &removedNode);
    void nodeReparented(const ModelNode &node, const NodeAbstractProperty &newPropertyParent, const NodeAbstractProperty &oldPropertyParent, AbstractView::PropertyChangeFlags propertyChange);
    void nodeSlidedToIndex(const NodeListProperty &listProperty, int newIndex, int oldIndex);

    // QmlModelView
    void stateChanged(const QmlModelState &newQmlModelState, const QmlModelState &oldQmlModelState);

    void customNotification(const AbstractView *view, const QString &identifier, const QList<ModelNode> &nodeList, const QList<QVariant> &data);


protected:
    void timerEvent(QTimerEvent*);

private slots:
    void sceneChanged();

private:
    void insertModelState(int i, const QmlModelState &state);
    void removeModelState(const QmlModelState &state);
    void clearModelStates();
    int modelStateIndex(const QmlModelState &state);

    void startUpdateTimer(int i, int offset);

    QList<QmlModelState> m_modelStates;
    StatesEditorModel *m_editorModel;
    QmlItemNode m_stateRootNode;

    QList<int> m_updateTimerIdList;
    QmlModelState m_oldRewriterAmendState;
};

} // namespace Internal
} // namespace QmlDesigner

#endif // STATESEDITORVIEW_H
