/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
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

#ifndef MODELNODECONTEXTMENU_H
#define MODELNODECONTEXTMENU_H

#include <QObject>
#include <QPoint>
#include <QAction>
#include <QCoreApplication>
#include <QMenu>
#include <qmlmodelview.h>

namespace QmlDesigner {

class ModelNodeAction : public QAction
{
     Q_OBJECT
public:
     enum ModelNodeActionType {
         SelectModelNode,
         DeSelectModelNode,
         CutSelection,
         CopySelection,
         DeleteSelection,
         ToFront,
         ToBack,
         Raise,
         Lower,
         Paste,
         Undo,
         Redo,
         ModelNodeVisibility,
         ResetSize,
         ResetPosition,
         GoIntoComponent,
         SetId,
         ResetZ,
         AnchorReset,
         AnchorFill,
         LayoutRow,
         LayoutColumn,
         LayoutGrid,
         LayoutFlow
     };


     ModelNodeAction( const QString & text, QObject *parent, QmlModelView *view,  const QList<ModelNode> &modelNodeList, ModelNodeActionType type);

     static void goIntoComponent(const ModelNode &modelNode);

public slots:
     void actionTriggered(bool);

private:
     void select();
     void deSelect();
     void cut();
     void copy();
     void deleteSelection();
     void toFront();
     void toBack();
     void raise();
     void lower();
     void paste();
     void undo();
     void redo();
     void setVisible(bool);
     void resetSize();
     void resetPosition();
     void goIntoComponent();
     void setId();
     void resetZ();
     void anchorsFill();
     void anchorsReset();
     void layoutRow();
     void layoutColumn();
     void layoutGrid();
     void layoutFlow();

     QmlModelView *m_view;
     QList<ModelNode> m_modelNodeList;
     ModelNodeActionType m_type;
};

class ModelNodeContextMenu
{
    Q_DECLARE_TR_FUNCTIONS(QmlDesigner::ModelNodeContextMenu)
public:
    ModelNodeContextMenu(QmlModelView *view);
    void execute(const QPoint &pos, bool selectionMenu);
    void setScenePos(const QPoint &pos);

    static void showContextMenu(QmlModelView *view, const QPoint &globalPosition, const QPoint &scenePosition, bool showSelection);

private:
    ModelNodeAction* createModelNodeAction(const QString &description, QMenu *menu, const QList<ModelNode> &modelNodeList, ModelNodeAction::ModelNodeActionType type, bool enabled = true);

    QmlModelView *m_view;
    QPoint m_scenePos;

};


};

#endif // MODELNODECONTEXTMENU_H
