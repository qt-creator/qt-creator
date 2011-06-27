#ifndef MODELNODECONTEXTMENU_H
#define MODELNODECONTEXTMENU_H

#include <QObject>
#include <QPoint>
#include <QAction>
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
         EnterComponent,
         SetId,
         ResetZ
     };


     ModelNodeAction( const QString & text, QObject *parent, QmlModelView *view,  const QList<ModelNode> &modelNodeList, ModelNodeActionType type);

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
     void enterComponent();
     void setId();
     void resetZ();

     QmlModelView *m_view;
     QList<ModelNode> m_modelNodeList;
     ModelNodeActionType m_type;
};

class ModelNodeContextMenu
{
public:
    ModelNodeContextMenu(QmlModelView *view);
    void execute(const QPoint &pos, bool selecetionMenu);
    void setScenePos(const QPoint &pos);

signals:

public slots:

private:
    ModelNodeAction* createModelNodeAction(const QString &description, QMenu *menu, const QList<ModelNode> &modelNodeList, ModelNodeAction::ModelNodeActionType type, bool enabled = true);

    QmlModelView *m_view;
    QPoint m_scenePos;

};


};

#endif // MODELNODECONTEXTMENU_H
