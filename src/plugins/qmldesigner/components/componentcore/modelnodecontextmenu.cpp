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

#include "modelnodecontextmenu.h"
#include "modelnodecontextmenu_helper.h"
#include "designeractionmanager.h"
#include <qmldesignerplugin.h>

#include <modelnode.h>

#include <utils/algorithm.h>

#include <QSet>

namespace QmlDesigner {

ModelNodeContextMenu::ModelNodeContextMenu(AbstractView *view) :
    m_selectionContext(view)
{
}

static QSet<ActionInterface* > findMembers(QSet<ActionInterface* > actionInterface,
                                                          const QByteArray &category)
{
    QSet<ActionInterface* > ret;

     foreach (ActionInterface* factory, actionInterface) {
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

    QList<ActionInterface* > matchingFactoriesList = matchingFactories.toList();
    Utils::sort(matchingFactoriesList, [](ActionInterface *l, ActionInterface *r) {
        return l->priority() > r->priority();
    });

    foreach (ActionInterface* actionInterface, matchingFactoriesList) {
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
           action->setIconVisibleInMenu(false);
           menu->addAction(action);
       }
    }
}

void ModelNodeContextMenu::execute(const QPoint &position, bool selectionMenuBool)
{
    QMenu* mainMenu = new QMenu();

    m_selectionContext.setShowSelectionTools(selectionMenuBool);
    m_selectionContext.setScenePosition(m_scenePos);


     QSet<ActionInterface* > factories =
             QSet<ActionInterface* >::fromList(QmlDesignerPlugin::instance()->designerActionManager().designerActions());

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
