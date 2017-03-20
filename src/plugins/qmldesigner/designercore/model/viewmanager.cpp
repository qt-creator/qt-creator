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

#include "viewmanager.h"

#ifndef QMLDESIGNER_TEST

#include <componentaction.h>
#include <designmodewidget.h>
#include <crumblebar.h>
#include <abstractview.h>
#include <rewriterview.h>
#include <nodeinstanceview.h>
#include <itemlibraryview.h>
#include <navigatorview.h>
#include <stateseditorview.h>
#include <formeditorview.h>
#include <texteditorview.h>
#include <propertyeditorview.h>
#include <componentview.h>
#include <debugview.h>
#include <importmanagerview.h>
#include <designeractionmanagerview.h>
#include <qmldesignerplugin.h>

#include <utils/algorithm.h>

#include <QLoggingCategory>
#include <QTabWidget>

namespace QmlDesigner {

static Q_LOGGING_CATEGORY(viewBenchmark, "qtc.viewmanager.attach")

class ViewManagerData
{
public:
    QmlModelState savedState;
    Internal::DebugView debugView;
    DesignerActionManagerView designerActionManagerView;
    NodeInstanceView nodeInstanceView;
    ComponentView componentView;
    FormEditorView formEditorView;
    TextEditorView textEditorView;
    ItemLibraryView itemLibraryView;
    NavigatorView navigatorView;
    PropertyEditorView propertyEditorView;
    StatesEditorView statesEditorView;

    QList<QPointer<AbstractView> > additionalViews;
};


static CrumbleBar *crumbleBar() {
    return QmlDesignerPlugin::instance()->mainWidget()->crumbleBar();
}

ViewManager::ViewManager()
    : d(new ViewManagerData)
{
    d->formEditorView.setGotoErrorCallback([this](int line, int column) {
        d->textEditorView.gotoCursorPosition(line, column);
        if (Internal::DesignModeWidget *designModeWidget = QmlDesignerPlugin::instance()->mainWidget())
            designModeWidget->showInternalTextEditor();
    });
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

    QTime time;
    if (viewBenchmark().isInfoEnabled())
        time.start();

    qCInfo(viewBenchmark) << Q_FUNC_INFO;

    setNodeInstanceViewKit(currentDesignDocument()->currentKit());
    currentModel()->setNodeInstanceView(&d->nodeInstanceView);

     qCInfo(viewBenchmark) << "NodeInstanceView:" << time.elapsed();
}

void ViewManager::attachRewriterView()
{
    QTime time;
    if (viewBenchmark().isInfoEnabled())
        time.start();

    qCInfo(viewBenchmark) << Q_FUNC_INFO;

    if (RewriterView *view = currentDesignDocument()->rewriterView()) {
        view->setWidgetStatusCallback([this](bool enable) {
            if (enable)
                enableWidgets();
            else
                disableWidgets();
        });

        currentModel()->setRewriterView(view);
        view->reactivateTextMofifierChangeSignals();
    }

    qCInfo(viewBenchmark) << "RewriterView:" << time.elapsed();
}

void ViewManager::detachRewriterView()
{
    if (RewriterView *view = currentDesignDocument()->rewriterView()) {
        view->deactivateTextMofifierChangeSignals();
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
    currentModel()->detachView(&d->textEditorView);
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
    setItemLibraryViewResourcePath(currentDesignDocument()->fileName().toFileInfo().absolutePath());
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
    QObject::connect(d->componentView.action(), &ComponentAction::currentComponentChanged,
                     currentDesignDocument(), &DesignDocument::changeToSubComponent);
    QObject::connect(d->componentView.action(), &ComponentAction::changedToMaster,
                     currentDesignDocument(), &DesignDocument::changeToMaster);
}

void ViewManager::detachComponentView()
{
    QObject::disconnect(d->componentView.action(), &ComponentAction::currentComponentChanged,
                        currentDesignDocument(), &DesignDocument::changeToSubComponent);
    QObject::disconnect(d->componentView.action(), &ComponentAction::changedToMaster,
                        currentDesignDocument(), &DesignDocument::changeToMaster);

    documentModel()->detachView(&d->componentView);
}

void ViewManager::attachViewsExceptRewriterAndComponetView()
{
    if (QmlDesignerPlugin::instance()->settings().value(
            DesignerSettingsKey::ENABLE_DEBUGVIEW).toBool())
        currentModel()->attachView(&d->debugView);

    attachNodeInstanceView();

    QTime time;
    if (viewBenchmark().isInfoEnabled())
        time.start();

    qCInfo(viewBenchmark) << Q_FUNC_INFO;

    currentModel()->attachView(&d->designerActionManagerView);

    int last = time.elapsed();
    qCInfo(viewBenchmark) << "ActionManagerView:" << last << time.elapsed();

    currentModel()->attachView(&d->formEditorView);

    int currentTime = time.elapsed();
    qCInfo(viewBenchmark) << "FormEditorView:" << currentTime - last;
    last = currentTime;

    currentModel()->attachView(&d->textEditorView);

    currentTime = time.elapsed();
    qCInfo(viewBenchmark) << "TextEditorView:" << currentTime - last;
    last = currentTime;

    currentModel()->attachView(&d->navigatorView);

    currentTime = time.elapsed();
    qCInfo(viewBenchmark) << "NavigatorView:" << currentTime - last;
    last = currentTime;

    attachItemLibraryView();

    currentTime = time.elapsed();
    qCInfo(viewBenchmark) << "ItemLibraryView:" << currentTime - last;
    last = currentTime;

    currentModel()->attachView(&d->statesEditorView);

    currentTime = time.elapsed();
    qCInfo(viewBenchmark) << "StatesEditorView:" << currentTime - last;
    last = currentTime;

    currentModel()->attachView(&d->propertyEditorView);

    currentTime = time.elapsed();
    qCInfo(viewBenchmark) << "PropertyEditorView:" << currentTime - last;
    last = currentTime;

    attachAdditionalViews();

    currentTime = time.elapsed();
    qCInfo(viewBenchmark) << "AdditionalViews:" << currentTime - last;
    last = currentTime;

    currentTime = time.elapsed();
    qCInfo(viewBenchmark) << "All:" << time.elapsed();
    last = currentTime;

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

void QmlDesigner::ViewManager::setNodeInstanceViewProject(ProjectExplorer::Project *project)
{
    d->nodeInstanceView.setProject(project);
}

QList<WidgetInfo> ViewManager::widgetInfos() const
{
    QList<WidgetInfo> widgetInfoList;

    widgetInfoList.append(d->formEditorView.widgetInfo());
    widgetInfoList.append(d->textEditorView.widgetInfo());
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

QWidget *ViewManager::widget(const QString &uniqueId) const
{
    foreach (const WidgetInfo &widgetInfo, widgetInfos()) {
        if (widgetInfo.uniqueId == uniqueId)
            return widgetInfo.widget;
    }
    return nullptr;
}

void ViewManager::disableWidgets()
{
    foreach (const WidgetInfo &widgetInfo, widgetInfos())
        if (widgetInfo.widgetFlags == DesignerWidgetFlags::DisableOnError)
            widgetInfo.widget->setEnabled(false);
}

void ViewManager::enableWidgets()
{
    foreach (const WidgetInfo &widgetInfo, widgetInfos())
        if (widgetInfo.widgetFlags == DesignerWidgetFlags::DisableOnError)
            widgetInfo.widget->setEnabled(true);
}

void ViewManager::pushFileOnCrumbleBar(const Utils::FileName &fileName)
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

void ViewManager::toggleStatesViewExpanded()
{
    d->statesEditorView.toggleStatesViewExpanded();
}

Model *ViewManager::currentModel() const
{
    return currentDesignDocument()->currentModel();
}

Model *ViewManager::documentModel() const
{
    return currentDesignDocument()->documentModel();
}

void ViewManager::exportAsImage()
{
    d->formEditorView.exportAsImage();
}

void ViewManager::reformatFileUsingTextEditorView()
{
    d->textEditorView.reformatFile();
}

} // namespace QmlDesigner

#endif //QMLDESIGNER_TEST
