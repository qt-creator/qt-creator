// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "modelnodecontextmenu.h"
#include "componentcoretracing.h"
#include "designeractionmanager.h"
#include "modelnodecontextmenu_helper.h"
#include "qmleditormenu.h"

#include <qmldesignerplugin.h>

#include <modelnode.h>

#include <utils/algorithm.h>

#include <QSet>

namespace QmlDesigner {

using NanotraceHR::keyValue;
using QmlDesigner::ComponentCoreTracing::category;

ModelNodeContextMenu::ModelNodeContextMenu(AbstractView *view) :
    m_selectionContext(view)
{
    NanotraceHR::Tracer tracer{"model node context menu contructor", category()};
}

namespace {
QSet<ActionInterface *> findMembers(const QSet<ActionInterface *> actionInterface,
                                    const QByteArray &category)
{
    NanotraceHR::Tracer tracer{"model node context menu find members",
                               ComponentCoreTracing::category()};

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
    NanotraceHR::Tracer tracer{"model node context menu populate menu",
                               ComponentCoreTracing::category()};

    QSet<ActionInterface *> matchingFactories = findMembers(actionInterfaces, category);

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

} // namespace

void ModelNodeContextMenu::execute(const QPoint &position, bool selectionMenuBool)
{
    NanotraceHR::Tracer tracer{"model node context menu execute", category()};

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
    NanotraceHR::Tracer tracer{"model node context menu set scene pos", category()};

    m_scenePos = position;
}

void ModelNodeContextMenu::showContextMenu(AbstractView *view,
                                           const QPoint &globalPosition,
                                           const QPoint &scenePosition,
                                           bool showSelection)
{
    NanotraceHR::Tracer tracer{"model node context menu show context menu", category()};

    ModelNodeContextMenu contextMenu(view);
    contextMenu.setScenePos(scenePosition);
    contextMenu.execute(globalPosition, showSelection);
}

} //QmlDesigner
