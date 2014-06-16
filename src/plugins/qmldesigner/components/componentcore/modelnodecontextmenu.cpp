/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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

static QSet<AbstractDesignerAction* > findMembers(QSet<AbstractDesignerAction* > designerActionSet,
                                                          const QString &category)
{
    QSet<AbstractDesignerAction* > ret;

     foreach (AbstractDesignerAction* factory, designerActionSet) {
         if (factory->category() == category)
             ret.insert(factory);
     }
     return ret;
}


void populateMenu(QSet<AbstractDesignerAction* > &abstractDesignerActions,
                  const QString &category,
                  QMenu* menu,
                  const SelectionContext &selectionContext)
{
    QSet<AbstractDesignerAction* > matchingFactories = findMembers(abstractDesignerActions, category);

    abstractDesignerActions.subtract(matchingFactories);

    QList<AbstractDesignerAction* > matchingFactoriesList = matchingFactories.toList();
    Utils::sort(matchingFactoriesList, [](AbstractDesignerAction *l, AbstractDesignerAction *r) {
        return l->priority() > r->priority();
    });

    foreach (AbstractDesignerAction* designerAction, matchingFactoriesList) {
       if (designerAction->type() == AbstractDesignerAction::Menu) {
           designerAction->currentContextChanged(selectionContext);
           QMenu *newMenu = designerAction->action()->menu();
           menu->addMenu(newMenu);

           //recurse

           populateMenu(abstractDesignerActions, designerAction->menuId(), newMenu, selectionContext);
       } else if (designerAction->type() == AbstractDesignerAction::Action) {
           QAction* action = designerAction->action();
           designerAction->currentContextChanged(selectionContext);
           menu->addAction(action);
       }
    }
}

void ModelNodeContextMenu::execute(const QPoint &position, bool selectionMenuBool)
{
    QMenu* mainMenu = new QMenu();

    m_selectionContext.setShowSelectionTools(selectionMenuBool);
    m_selectionContext.setScenePosition(m_scenePos);


     QSet<AbstractDesignerAction* > factories =
             QSet<AbstractDesignerAction* >::fromList(QmlDesignerPlugin::instance()->designerActionManager().designerActions());

     populateMenu(factories, QString(""), mainMenu, m_selectionContext);

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
