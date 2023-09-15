// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "viewmanager.h"
#include "modelnodecontextmenu_helper.h"

#include <abstractview.h>
#include <assetslibraryview.h>
#include <capturingconnectionmanager.h>
#include <collectionview.h>
#include <componentaction.h>
#include <componentview.h>
#include <contentlibraryview.h>
#include <crumblebar.h>
#include <debugview.h>
#include <designeractionmanagerview.h>
#include <designmodewidget.h>
#include <edit3dview.h>
#include <formeditorview.h>
#include <itemlibraryview.h>
#include <materialbrowserview.h>
#include <materialeditorview.h>
#include <navigatorview.h>
#include <nodeinstanceview.h>
#include <propertyeditorview.h>
#include <rewriterview.h>
#include <stateseditorview.h>
#include <texteditorview.h>
#include <textureeditorview.h>
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
    ViewManagerData(AsynchronousImageCache &imageCache,
                    ExternalDependenciesInterface &externalDependencies)
        : debugView{externalDependencies}
        , designerActionManagerView{externalDependencies}
        , nodeInstanceView(QCoreApplication::arguments().contains("-capture-puppet-stream")
                               ? capturingConnectionManager
                               : connectionManager,
                           externalDependencies,
                           true)
        , collectionView{externalDependencies}
        , contentLibraryView{externalDependencies}
        , componentView{externalDependencies}
        , edit3DView{externalDependencies}
        , formEditorView{externalDependencies}
        , textEditorView{externalDependencies}
        , assetsLibraryView{externalDependencies}
        , itemLibraryView(imageCache, externalDependencies)
        , navigatorView{externalDependencies}
        , propertyEditorView(imageCache, externalDependencies)
        , materialEditorView{externalDependencies}
        , materialBrowserView{imageCache, externalDependencies}
        , textureEditorView{imageCache, externalDependencies}
        , statesEditorView{externalDependencies}
    {}

    InteractiveConnectionManager connectionManager;
    CapturingConnectionManager capturingConnectionManager;
    QmlModelState savedState;
    Internal::DebugView debugView;
    DesignerActionManagerView designerActionManagerView;
    NodeInstanceView nodeInstanceView;
    CollectionView collectionView;
    ContentLibraryView contentLibraryView;
    ComponentView componentView;
    Edit3DView edit3DView;
    FormEditorView formEditorView;
    TextEditorView textEditorView;
    AssetsLibraryView assetsLibraryView;
    ItemLibraryView itemLibraryView;
    NavigatorView navigatorView;
    PropertyEditorView propertyEditorView;
    MaterialEditorView materialEditorView;
    MaterialBrowserView materialBrowserView;
    TextureEditorView textureEditorView;
    StatesEditorView statesEditorView;

    std::vector<std::unique_ptr<AbstractView>> additionalViews;
    bool disableStandardViews = false;
};

static CrumbleBar *crumbleBar() {
    return QmlDesignerPlugin::instance()->mainWidget()->crumbleBar();
}

ViewManager::ViewManager(AsynchronousImageCache &imageCache,
                         ExternalDependenciesInterface &externalDependencies)
    : d(std::make_unique<ViewManagerData>(imageCache, externalDependencies))
{
    d->formEditorView.setGotoErrorCallback([this](int line, int column) {
        d->textEditorView.gotoCursorPosition(line, column);
        if (Internal::DesignModeWidget *designModeWidget = QmlDesignerPlugin::instance()
                                                               ->mainWidget())
            designModeWidget->showDockWidget("TextEditor");
    });

    registerNanotraceActions();
}

ViewManager::~ViewManager() = default;

DesignDocument *ViewManager::currentDesignDocument() const
{
    return QmlDesignerPlugin::instance()->documentManager().currentDesignDocument();
}

void ViewManager::attachNodeInstanceView()
{
    if (d->nodeInstanceView.isAttached())
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

QList<AbstractView *> ViewManager::views() const
{
    auto list = Utils::transform<QList<AbstractView *>>(d->additionalViews,
                                                        [](auto &&view) { return view.get(); });
    list.append(standardViews());
    return list;
}

QList<AbstractView *> ViewManager::standardViews() const
{
    QList<AbstractView *> list = {&d->edit3DView,
                                  &d->formEditorView,
                                  &d->textEditorView,
                                  &d->assetsLibraryView,
                                  &d->itemLibraryView,
                                  &d->navigatorView,
                                  &d->propertyEditorView,
                                  &d->materialEditorView,
                                  &d->materialBrowserView,
                                  &d->textureEditorView,
                                  &d->statesEditorView,
                                  &d->designerActionManagerView};

    if (QmlDesignerPlugin::instance()
            ->settings()
            .value(DesignerSettingsKey::ENABLE_DEBUGVIEW)
            .toBool())
        list.append(&d->debugView);

    if (qEnvironmentVariableIsSet("ENABLE_QDS_COLLECTIONVIEW"))
        list.append(&d->collectionView);

#ifdef CHECK_LICENSE
    if (checkLicense() == FoundLicense::enterprise)
        list.append(&d->contentLibraryView);
#else
    list.append(&d->contentLibraryView);
#endif

    return list;
}

void ViewManager::registerNanotraceActions()
{
    if constexpr (isNanotraceEnabled()) {
        auto handleShutdownNanotraceAction = [](const SelectionContext &) {};
        auto shutdownNanotraceIcon = []() { return QIcon(); };
        auto startNanotraceAction = new ModelNodeAction("Start Nanotrace",
                                                        QObject::tr("Start Nanotrace"),
                                                        shutdownNanotraceIcon(),
                                                        QObject::tr("Start Nanotrace"),
                                                        ComponentCoreConstants::eventListCategory,
                                                        QKeySequence(),
                                                        22,
                                                        handleShutdownNanotraceAction);

        QObject::connect(startNanotraceAction->action(), &QAction::triggered, [&]() {
            d->nodeInstanceView.startNanotrace();
        });

        d->designerActionManagerView.designerActionManager().addDesignerAction(startNanotraceAction);

        auto shutDownNanotraceAction = new ModelNodeAction("ShutDown Nanotrace",
                                                           QObject::tr("Shut Down Nanotrace"),
                                                           shutdownNanotraceIcon(),
                                                           QObject::tr("Shut Down Nanotrace"),
                                                           ComponentCoreConstants::eventListCategory,
                                                           QKeySequence(),
                                                           23,
                                                           handleShutdownNanotraceAction);

        QObject::connect(shutDownNanotraceAction->action(), &QAction::triggered, [&]() {
            d->nodeInstanceView.endNanotrace();
        });

        d->designerActionManagerView.designerActionManager().addDesignerAction(shutDownNanotraceAction);
    }
}

void ViewManager::resetPropertyEditorView()
{
    d->propertyEditorView.resetView();
}

void ViewManager::registerFormEditorTool(std::unique_ptr<AbstractCustomTool> &&tool)
{
    d->formEditorView.registerTool(std::move(tool));
}

void ViewManager::detachViewsExceptRewriterAndComponetView()
{
    switchStateEditorViewToBaseState();
    detachAdditionalViews();

    detachStandardViews();

    currentModel()->setNodeInstanceView(nullptr);
}

void ViewManager::attachAdditionalViews()
{
    for (auto &view : d->additionalViews)
        currentModel()->attachView(view.get());
}

void ViewManager::detachAdditionalViews()
{
    for (auto &view : d->additionalViews)
        currentModel()->detachView(view.get());
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
    widgetInfoList.append(d->assetsLibraryView.widgetInfo());
    widgetInfoList.append(d->itemLibraryView.widgetInfo());
    widgetInfoList.append(d->navigatorView.widgetInfo());
    widgetInfoList.append(d->propertyEditorView.widgetInfo());
    widgetInfoList.append(d->materialEditorView.widgetInfo());
    widgetInfoList.append(d->materialBrowserView.widgetInfo());
    widgetInfoList.append(d->textureEditorView.widgetInfo());
    widgetInfoList.append(d->statesEditorView.widgetInfo());

    if (qEnvironmentVariableIsSet("ENABLE_QDS_COLLECTIONVIEW"))
        widgetInfoList.append(d->collectionView.widgetInfo());

#ifdef CHECK_LICENSE
    if (checkLicense() == FoundLicense::enterprise)
        widgetInfoList.append(d->contentLibraryView.widgetInfo());
#else
    widgetInfoList.append(d->contentLibraryView.widgetInfo());
#endif

    if (d->debugView.hasWidget())
        widgetInfoList.append(d->debugView.widgetInfo());

    for (auto &view : d->additionalViews) {
        if (view->hasWidget())
            widgetInfoList.append(view->widgetInfo());
    }

    Utils::sort(widgetInfoList, [](const WidgetInfo &firstWidgetInfo, const WidgetInfo &secondWidgetInfo) {
        return firstWidgetInfo.placementPriority < secondWidgetInfo.placementPriority;
    });

    return widgetInfoList;
}

QWidget *ViewManager::widget(const QString &uniqueId) const
{
    const QList<WidgetInfo> widgetInfoList = widgetInfos();
    for (const WidgetInfo &widgetInfo : widgetInfoList) {
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

const AbstractView *ViewManager::view() const
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

QImage ViewManager::takeFormEditorScreenshot()
{
    return d->formEditorView.takeFormEditorScreenshot();
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

void ViewManager::addView(std::unique_ptr<AbstractView> &&view)
{
    d->additionalViews.push_back(std::move(view));
}

} // namespace QmlDesigner
