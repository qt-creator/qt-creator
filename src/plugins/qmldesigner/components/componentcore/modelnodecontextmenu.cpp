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

#include "modelnodecontextmenu.h"
#include "modelnodecontextmenu_helper.h"
#include "designeractionmanager.h"

#include <cmath>
#include <QApplication>
#include <QMessageBox>
#include <coreplugin/editormanager/editormanager.h>
#include <nodeabstractproperty.h>
#include <nodelistproperty.h>
#include <nodemetainfo.h>
#include <modelnode.h>
#include <qmlitemnode.h>
#include <variantproperty.h>
#include <bindingproperty.h>
#include <nodeproperty.h>
#include <rewritingexception.h>
#include <rewritertransaction.h>
#include <designmodewidget.h>
#include <qmlanchors.h>

#include <QSet>

namespace QmlDesigner {

ModelNodeContextMenu::ModelNodeContextMenu(QmlModelView *view) :
    m_view(view),
    m_selectionContext(view)
{
}

static bool sortFunction(AbstractDesignerAction * abstractDesignerAction01, AbstractDesignerAction *abstractDesignerAction02)
{
    return abstractDesignerAction01->priority() > abstractDesignerAction02->priority();
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
    qSort(matchingFactoriesList.begin(), matchingFactoriesList.end(), &sortFunction);

    foreach (AbstractDesignerAction* designerAction, matchingFactoriesList) {
       if (designerAction->type() == AbstractDesignerAction::Menu) {
           designerAction->setCurrentContext(selectionContext);
           QMenu *newMenu = designerAction->action()->menu();
           menu->addMenu(newMenu);

           //recurse

           populateMenu(abstractDesignerActions, designerAction->menuId(), newMenu, selectionContext);
       } else if (designerAction->type() == AbstractDesignerAction::Action) {
           QAction* action = designerAction->action();
           designerAction->setCurrentContext(selectionContext);
           menu->addAction(action);
       }
    }
}

void ModelNodeContextMenu::execute(const QPoint &position, bool selectionMenuBool)
{
    QMenu* mainMenu = new QMenu();

    m_selectionContext.setShowSelectionTools(selectionMenuBool);
    m_selectionContext.setScenePos(m_scenePos);


     QSet<AbstractDesignerAction* > factories =
             QSet<AbstractDesignerAction* >::fromList(DesignerActionManager::designerActions());

     populateMenu(factories, QString(""), mainMenu, m_selectionContext);

     mainMenu->exec(position);
     mainMenu->deleteLater();
}

void ModelNodeContextMenu::setScenePos(const QPoint &position)
{
    m_scenePos = position;
}

void ModelNodeContextMenu::showContextMenu(QmlModelView *view,
                                           const QPoint &globalPosition,
                                           const QPoint &scenePosition,
                                           bool showSelection)
{
    ModelNodeContextMenu contextMenu(view);
    contextMenu.setScenePos(scenePosition);
    contextMenu.execute(globalPosition, showSelection);
}

} //QmlDesigner
