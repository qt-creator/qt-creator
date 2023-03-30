// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "modelnodecontextmenu.h"
#include "modelnodecontextmenu_helper.h"
#include "designeractionmanager.h"
#include <qmldesignerplugin.h>
#include "qmleditormenu.h"

#include <modelnode.h>

#include <utils/algorithm.h>

#include <QSet>

namespace QmlDesigner {

ModelNodeContextMenu::ModelNodeContextMenu(AbstractView *view) :
    m_selectionContext(view)
{
}

static QSet<ActionInterface *> findMembers(const QSet<ActionInterface *> actionInterface,
                                           const QByteArray &category)
{
    QSet<ActionInterface *> ret;

    for (ActionInterface *factory : actionInterface) {
        if (factory->category() == category)
            ret.insert(factory);
    }
    return ret;
}

void populateMenu(QSet<ActionInterface* > &actionInterfaces,
                  const QByteArray &category,
                  QMenu* menu,
                  const SelectionContext &selectionContext)
{
    QSet<ActionInterface* > matchingFactories = findMembers(actionInterfaces, category);

    actionInterfaces.subtract(matchingFactories);

    QList<ActionInterface* > matchingFactoriesList = Utils::toList(matchingFactories);
    Utils::sort(matchingFactoriesList, [](ActionInterface *l, ActionInterface *r) {
        return l->priority() < r->priority();
    });

    for (ActionInterface* actionInterface : std::as_const(matchingFactoriesList)) {
        if (actionInterface->type() == ActionInterface::ContextMenu) {
            actionInterface->currentContextChanged(selectionContext);
            QMenu *newMenu = actionInterface->action()->menu();
            if (newMenu && !newMenu->title().isEmpty())
                menu->addMenu(newMenu);

            //recurse

            populateMenu(actionInterfaces, actionInterface->menuId(), newMenu, selectionContext);
        } else if (actionInterface->type() == ActionInterface::ContextMenuAction
                   || actionInterface->type() == ActionInterface::Action
                   || actionInterface->type() == ActionInterface::FormEditorAction) {
           QAction* action = actionInterface->action();
           actionInterface->currentContextChanged(selectionContext);
           action->setIconVisibleInMenu(true);
           menu->addAction(action);
       }
    }
}

void ModelNodeContextMenu::execute(const QPoint &position, bool selectionMenuBool)
{
    auto mainMenu = new QmlEditorMenu();

    m_selectionContext.setShowSelectionTools(selectionMenuBool);
    m_selectionContext.setScenePosition(m_scenePos);

    auto &manager = QmlDesignerPlugin::instance()->designerActionManager();

    manager.setupContext();

    QSet<ActionInterface* > factories = Utils::toSet(manager.designerActions());

     populateMenu(factories, QByteArray(), mainMenu, m_selectionContext);

     mainMenu->exec(position);
     mainMenu->deleteLater();
}

void ModelNodeContextMenu::setScenePos(const QPoint &position)
{
    m_scenePos = position;
}

void ModelNodeContextMenu::showContextMenu(AbstractView *view,
                                           const QPoint &globalPosition,
                                           const QPoint &scenePosition,
                                           bool showSelection)
{
    ModelNodeContextMenu contextMenu(view);
    contextMenu.setScenePos(scenePosition);
    contextMenu.execute(globalPosition, showSelection);
}

} //QmlDesigner
