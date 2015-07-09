/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "viewmanager.h"

#include <rewriterview.h>
#include <nodeinstanceview.h>
#include <itemlibraryview.h>
#include <navigatorview.h>
#include <stateseditorview.h>
#include <formeditorview.h>
#include <propertyeditorview.h>
#include <componentview.h>
#include <debugview.h>
#include <importmanagerview.h>
#include <designeractionmanagerview.h>

#include "componentaction.h"
#include "designmodewidget.h"
#include "crumblebar.h"

#include <qmldesigner/qmldesignerplugin.h>
#include <utils/algorithm.h>


namespace QmlDesigner {

class ViewManagerData
{
public:
    QmlModelState savedState;
    Internal::DebugView debugView;
    ComponentView componentView;
    FormEditorView formEditorView;
    ItemLibraryView itemLibraryView;
    NavigatorView navigatorView;
    PropertyEditorView propertyEditorView;
    StatesEditorView statesEditorView;
    NodeInstanceView nodeInstanceView;
    DesignerActionManagerView designerActionManagerView;

    QList<QPointer<AbstractView> > additionalViews;
};


static CrumbleBar *crumbleBar() {
    return QmlDesignerPlugin::instance()->mainWidget()->crumbleBar();
}

ViewManager::ViewManager()
    : d(new ViewManagerData)
{
}

ViewManager::~ViewManager()
{
    foreach (const QPointer<AbstractView> &view, d->additionalViews)
        delete view.data();

    delete d;
}

DesignDocument *ViewManager::currentDesignDocument() const
{
    return QmlDesignerPlugin::instance()->documentManager().currentDesignDocument();
}

void ViewManager::attachNodeInstanceView()
{
    setNodeInstanceViewKit(currentDesignDocument()->currentKit());
    currentModel()->setNodeInstanceView(&d->nodeInstanceView);
}

void ViewManager::attachRewriterView()
{
    if (currentDesignDocument()->rewriterView()) {
        currentModel()->setRewriterView(currentDesignDocument()->rewriterView());
        currentDesignDocument()->rewriterView()->reactivateTextMofifierChangeSignals();
    }
}

void ViewManager::detachRewriterView()
{
    if (currentDesignDocument()->rewriterView()) {
        currentDesignDocument()->rewriterView()->deactivateTextMofifierChangeSignals();
        currentModel()->setRewriterView(0);
    }
}

void ViewManager::switchStateEditorViewToBaseState()
{
    if (d->statesEditorView.isAttached()) {
        d->savedState = d->statesEditorView.currentState();
        d->statesEditorView.setCurrentState(d->statesEditorView.baseState());
    }
}

void ViewManager::switchStateEditorViewToSavedState()
{
    if (d->savedState.isValid() && d->statesEditorView.isAttached())
        d->statesEditorView.setCurrentState(d->savedState);
}

void ViewManager::resetPropertyEditorView()
{
    d->propertyEditorView.resetView();
}

void ViewManager::registerFormEditorToolTakingOwnership(AbstractCustomTool *tool)
{
    d->formEditorView.registerTool(tool);
}

void ViewManager::registerViewTakingOwnership(AbstractView *view)
{
    d->additionalViews.append(view);
}

void ViewManager::detachViewsExceptRewriterAndComponetView()
{
    switchStateEditorViewToBaseState();
    detachAdditionalViews();
    currentModel()->detachView(&d->designerActionManagerView);
    currentModel()->detachView(&d->formEditorView);
    currentModel()->detachView(&d->navigatorView);
    currentModel()->detachView(&d->itemLibraryView);
    currentModel()->detachView(&d->statesEditorView);
    currentModel()->detachView(&d->propertyEditorView);

    if (d->debugView.isAttached())
        currentModel()->detachView(&d->debugView);

    currentModel()->setNodeInstanceView(0);
}

void ViewManager::attachItemLibraryView()
{
    setItemLibraryViewResourcePath(QFileInfo(currentDesignDocument()->fileName().fileName()).absolutePath());
    currentModel()->attachView(&d->itemLibraryView);
}

void ViewManager::attachAdditionalViews()
{
    foreach (const QPointer<AbstractView> &view, d->additionalViews)
        currentModel()->attachView(view.data());
}

void ViewManager::detachAdditionalViews()
{
    foreach (const QPointer<AbstractView> &view, d->additionalViews)
        currentModel()->detachView(view.data());
}

void ViewManager::attachComponentView()
{
    documentModel()->attachView(&d->componentView);
    QObject::connect(d->componentView.action(), SIGNAL(currentComponentChanged(ModelNode)), currentDesignDocument(), SLOT(changeToSubComponent(ModelNode)));
    QObject::connect(d->componentView.action(), SIGNAL(changedToMaster()), currentDesignDocument(), SLOT(changeToMaster()));
}

void ViewManager::detachComponentView()
{
    QObject::disconnect(d->componentView.action(), SIGNAL(currentComponentChanged(ModelNode)), currentDesignDocument(), SLOT(changeToSubComponent(ModelNode)));
    QObject::disconnect(d->componentView.action(), SIGNAL(changedToMaster()), currentDesignDocument(), SLOT(changeToMaster()));

    documentModel()->detachView(&d->componentView);
}

void ViewManager::attachViewsExceptRewriterAndComponetView()
{
    if (QmlDesignerPlugin::instance()->settings().enableDebugView)
        currentModel()->attachView(&d->debugView);

    attachNodeInstanceView();
    currentModel()->attachView(&d->formEditorView);
    currentModel()->attachView(&d->navigatorView);
    attachItemLibraryView();
    currentModel()->attachView(&d->statesEditorView);
    currentModel()->attachView(&d->propertyEditorView);
    currentModel()->attachView(&d->designerActionManagerView);
    attachAdditionalViews();
    switchStateEditorViewToSavedState();
}

void ViewManager::setItemLibraryViewResourcePath(const QString &resourcePath)
{
    d->itemLibraryView.setResourcePath(resourcePath);
}

void ViewManager::setComponentNode(const ModelNode &componentNode)
{
    d->componentView.setComponentNode(componentNode);
}

void ViewManager::setComponentViewToMaster()
{
    d->componentView.setComponentToMaster();
}

void ViewManager::setNodeInstanceViewKit(ProjectExplorer::Kit *kit)
{
    d->nodeInstanceView.setKit(kit);
}

QList<WidgetInfo> ViewManager::widgetInfos()
{
    QList<WidgetInfo> widgetInfoList;

    widgetInfoList.append(d->formEditorView.widgetInfo());
    widgetInfoList.append(d->itemLibraryView.widgetInfo());
    widgetInfoList.append(d->navigatorView.widgetInfo());
    widgetInfoList.append(d->propertyEditorView.widgetInfo());
    widgetInfoList.append(d->statesEditorView.widgetInfo());
    if (d->debugView.hasWidget())
        widgetInfoList.append(d->debugView.widgetInfo());

    foreach (const QPointer<AbstractView> &abstractView, d->additionalViews) {
        if (abstractView && abstractView->hasWidget())
            widgetInfoList.append(abstractView->widgetInfo());
    }

    Utils::sort(widgetInfoList, [](const WidgetInfo &firstWidgetInfo, const WidgetInfo &secondWidgetInfo) {
        return firstWidgetInfo.placementPriority < secondWidgetInfo.placementPriority;
    });

    return widgetInfoList;
}

void ViewManager::disableWidgets()
{
    foreach (const WidgetInfo &widgetInfo, widgetInfos())
        widgetInfo.widget->setEnabled(false);
}

void ViewManager::enableWidgets()
{
    foreach (const WidgetInfo &widgetInfo, widgetInfos())
        widgetInfo.widget->setEnabled(true);
}

void ViewManager::pushFileOnCrumbleBar(const QString &fileName)
{
    crumbleBar()->pushFile(fileName);
}

void ViewManager::pushInFileComponentOnCrumbleBar(const ModelNode &modelNode)
{
    crumbleBar()->pushInFileComponent(modelNode);
}

void ViewManager::nextFileIsCalledInternally()
{
    crumbleBar()->nextFileIsCalledInternally();
}

NodeInstanceView *ViewManager::nodeInstanceView() const
{
    return &d->nodeInstanceView;
}

QWidgetAction *ViewManager::componentViewAction() const
{
    return d->componentView.action();
}

DesignerActionManager &ViewManager::designerActionManager()
{
    return d->designerActionManagerView.designerActionManager();
}

const DesignerActionManager &ViewManager::designerActionManager() const
{
    return d->designerActionManagerView.designerActionManager();
}

Model *ViewManager::currentModel() const
{
    return currentDesignDocument()->currentModel();
}

Model *ViewManager::documentModel() const
{
    return currentDesignDocument()->documentModel();
}

} // namespace QmlDesigner
