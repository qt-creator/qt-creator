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
#include <edit3dview.h>
#include <formeditorview.h>
#include <texteditorview.h>
#include <propertyeditorview.h>
#include <componentview.h>
#include <debugview.h>
#include <importmanagerview.h>
#include <designeractionmanagerview.h>
#include <qmldesignerplugin.h>

#include <utils/algorithm.h>

#include <QElapsedTimer>
#include <QLoggingCategory>
#include <QTabWidget>

namespace QmlDesigner {

static Q_LOGGING_CATEGORY(viewBenchmark, "qtc.viewmanager.attach", QtWarningMsg)

class ViewManagerData
{
public:
    QmlModelState savedState;
    Internal::DebugView debugView;
    DesignerActionManagerView designerActionManagerView;
    NodeInstanceView nodeInstanceView;
    ComponentView componentView;
    Edit3DView edit3DView;
    FormEditorView formEditorView;
    TextEditorView textEditorView;
    ItemLibraryView itemLibraryView;
    NavigatorView navigatorView;
    PropertyEditorView propertyEditorView;
    StatesEditorView statesEditorView;

    QList<QPointer<AbstractView> > additionalViews;
    bool disableStandardViews = false;
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
    if (nodeInstanceView()->isAttached())
        return;

    QElapsedTimer time;
    if (viewBenchmark().isInfoEnabled())
        time.start();

    qCInfo(viewBenchmark) << Q_FUNC_INFO;

    setNodeInstanceViewTarget(currentDesignDocument()->currentTarget());
    currentModel()->setNodeInstanceView(&d->nodeInstanceView);

     qCInfo(viewBenchmark) << "NodeInstanceView:" << time.elapsed();
}

void ViewManager::attachRewriterView()
{
    QElapsedTimer time;
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
        view->restoreAuxiliaryData();
    }

    qCInfo(viewBenchmark) << "RewriterView:" << time.elapsed();
}

void ViewManager::detachRewriterView()
{
    if (RewriterView *view = currentDesignDocument()->rewriterView()) {
        view->deactivateTextMofifierChangeSignals();
        currentModel()->setRewriterView(nullptr);
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

QList<QPointer<AbstractView> > ViewManager::views() const
{
    auto list = d->additionalViews;
    list.append(standardViews());
    return list;
}

const QList<QPointer<AbstractView> > ViewManager::standardViews() const
{
    QList<QPointer<AbstractView>> list = {
                    &d->edit3DView,
                    &d->formEditorView,
                    &d->textEditorView,
                    &d->itemLibraryView,
                    &d->navigatorView,
                    &d->propertyEditorView,
                    &d->statesEditorView,
                    &d->designerActionManagerView
                };

    if (QmlDesignerPlugin::instance()->settings().value(
                DesignerSettingsKey::ENABLE_DEBUGVIEW).toBool())
         list.append(&d->debugView);

    return list;
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

    detachStandardViews();

    currentModel()->setNodeInstanceView(nullptr);
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

void ViewManager::detachStandardViews()
{
    for (const auto &view : standardViews()) {
        if (view->isAttached())
            currentModel()->detachView(view);
    }
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

    QElapsedTimer time;
    if (viewBenchmark().isInfoEnabled())
        time.start();

    qCInfo(viewBenchmark) << Q_FUNC_INFO;

    int last = time.elapsed();
    int currentTime = 0;
    if (!d->disableStandardViews) {
        for (const auto &view : standardViews()) {
            currentModel()->attachView(view);
            currentTime = time.elapsed();
            qCInfo(viewBenchmark) << view->widgetInfo().uniqueId << currentTime - last;
            last = currentTime;
        }
    }

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

void ViewManager::setNodeInstanceViewTarget(ProjectExplorer::Target *target)
{
    d->nodeInstanceView.setTarget(target);
}

QList<WidgetInfo> ViewManager::widgetInfos() const
{
    QList<WidgetInfo> widgetInfoList;

    widgetInfoList.append(d->edit3DView.widgetInfo());
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
    for (const auto &view : views())
        view->disableWidget();
}

void ViewManager::enableWidgets()
{
    for (const auto &view : views())
        view->enableWidget();
}

void ViewManager::pushFileOnCrumbleBar(const Utils::FilePath &fileName)
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

void ViewManager::qmlJSEditorContextHelp(const Core::IContext::HelpCallback &callback) const
{
    d->textEditorView.qmlJSEditorContextHelp(callback);
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

bool ViewManager::usesRewriterView(RewriterView *rewriterView)
{
    return currentDesignDocument()->rewriterView() == rewriterView;
}

void ViewManager::disableStandardViews()
{
    d->disableStandardViews = true;
    detachStandardViews();
}

void ViewManager::enableStandardViews()
{
    d->disableStandardViews = false;
    attachViewsExceptRewriterAndComponetView();
}

} // namespace QmlDesigner

#endif //QMLDESIGNER_TEST
