// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "viewmanager.h"
#include "modelnodecontextmenu_helper.h"

#include <abstractview.h>
#include <assetslibraryview.h>
#include <capturingconnectionmanager.h>
#include <componentaction.h>
#include <componentview.h>
#include <contentlibraryview.h>
#include <crumblebar.h>
#include <debugview.h>
#include <designeractionmanagerview.h>
#include <designmodewidget.h>
#include <dynamiclicensecheck.h>
#include <edit3dview.h>
#include <formeditorview.h>
#include <itemlibraryview.h>
#include <materialbrowserview.h>
#include <materialeditorview.h>
#include <model/auxiliarypropertystorageview.h>
#include <navigatorview.h>
#include <nodeinstanceview.h>
#include <propertyeditorview.h>
#include <qmldesignerplugin.h>
#include <rewriterview.h>
#include <stateseditorview.h>
#include <texteditorview.h>
#include <textureeditorview.h>

#include <coreplugin/icore.h>

#include <sqlitedatabase.h>
#include <utils/algorithm.h>

#include <advanceddockingsystem/dockwidget.h>

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
        , auxiliaryDataKeyView{auxiliaryDataDatabase, externalDependencies}
        , designerActionManagerView{externalDependencies}
        , nodeInstanceView(QCoreApplication::arguments().contains("-capture-puppet-stream")
                               ? capturingConnectionManager
                               : connectionManager,
                           externalDependencies,
                           true)
        , contentLibraryView{imageCache, externalDependencies}
        , componentView{externalDependencies}
#ifndef QTC_USE_QML_DESIGNER_LITE
        , edit3DView{externalDependencies}
#endif
        , formEditorView{externalDependencies}
        , textEditorView{externalDependencies}
        , assetsLibraryView{externalDependencies}
        , itemLibraryView(imageCache, externalDependencies)
        , navigatorView{externalDependencies}
        , propertyEditorView(imageCache, externalDependencies)
#ifndef QTC_USE_QML_DESIGNER_LITE
        , materialEditorView{externalDependencies}
        , materialBrowserView{imageCache, externalDependencies}
        , textureEditorView{imageCache, externalDependencies}
#endif
        , statesEditorView{externalDependencies}
    {}

    InteractiveConnectionManager connectionManager;
    CapturingConnectionManager capturingConnectionManager;
    QmlModelState savedState;
    Internal::DebugView debugView;
    Sqlite::Database auxiliaryDataDatabase{
        Utils::PathString{Core::ICore::userResourcePath("auxiliary_data.db").toString()},
        Sqlite::JournalMode::Wal,
        Sqlite::LockingMode::Normal};
    AuxiliaryPropertyStorageView auxiliaryDataKeyView;
    DesignerActionManagerView designerActionManagerView;
    NodeInstanceView nodeInstanceView;
    ContentLibraryView contentLibraryView;
    ComponentView componentView;
#ifndef QTC_USE_QML_DESIGNER_LITE
    Edit3DView edit3DView;
#endif
    FormEditorView formEditorView;
    TextEditorView textEditorView;
    AssetsLibraryView assetsLibraryView;
    ItemLibraryView itemLibraryView;
    NavigatorView navigatorView;
    PropertyEditorView propertyEditorView;
#ifndef QTC_USE_QML_DESIGNER_LITE
    MaterialEditorView materialEditorView;
    MaterialBrowserView materialBrowserView;
    TextureEditorView textureEditorView;
#endif
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
        if (Internal::DesignModeWidget *w = QmlDesignerPlugin::instance()->mainWidget())
            w->showDockWidget("TextEditor");
        d->textEditorView.gotoCursorPosition(line, column);
    });

    registerViewActions();

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
        view->reactivateTextModifierChangeSignals();
        view->restoreAuxiliaryData();
    }

    qCInfo(viewBenchmark) << "RewriterView:" << time.elapsed();
}

void ViewManager::detachRewriterView()
{
    if (RewriterView *view = currentDesignDocument()->rewriterView()) {
        view->deactivateTextModifierChangeSignals();
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

void ViewManager::hideView(AbstractView &view)
{
    disableView(view);
    view.setVisibility(false);
}

void ViewManager::showView(AbstractView &view)
{
    view.setVisibility(true);
    enableView(view);
}

QList<AbstractView *> ViewManager::standardViews() const
{
#ifndef QTC_USE_QML_DESIGNER_LITE
    QList<AbstractView *> list = {&d->auxiliaryDataKeyView,
                                  &d->edit3DView,
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
#else
    QList<AbstractView *> list = {&d->formEditorView,
                                  &d->textEditorView,
                                  &d->assetsLibraryView,
                                  &d->itemLibraryView,
                                  &d->navigatorView,
                                  &d->propertyEditorView,
                                  &d->statesEditorView,
                                  &d->designerActionManagerView};
#endif

    if (QmlDesignerPlugin::instance()
            ->settings()
            .value(DesignerSettingsKey::ENABLE_DEBUGVIEW)
            .toBool())
        list.append(&d->debugView);

    if (checkEnterpriseLicense())
        list.append(&d->contentLibraryView);

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

void ViewManager::registerViewActions()
{
    for (auto view : views()) {
        if (view->hasWidget())
            registerViewAction(*view);
    }
}

void ViewManager::registerViewAction(AbstractView &view)
{
    auto viewAction = view.action();
    viewAction->setCheckable(true);
#ifdef DETACH_DISABLED_VIEWS
    QObject::connect(view.action(),
                     &AbstractViewAction::viewCheckedChanged,
                     [&](bool checked, AbstractView &view) {
                         if (checked)
                             enableView(view);
                         else
                             disableView(view);
                     });
#endif
}

void ViewManager::enableView(AbstractView &view)
{
    if (auto model = currentModel())
        model->attachView(&view);
}

void ViewManager::disableView(AbstractView &view)
{
    if (auto model = currentModel())
        model->detachView(&view);
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

#ifndef QTC_USE_QML_DESIGNER_LITE
    widgetInfoList.append(d->edit3DView.widgetInfo());
#endif
    widgetInfoList.append(d->formEditorView.widgetInfo());
    widgetInfoList.append(d->textEditorView.widgetInfo());
    widgetInfoList.append(d->assetsLibraryView.widgetInfo());
    widgetInfoList.append(d->itemLibraryView.widgetInfo());
    widgetInfoList.append(d->navigatorView.widgetInfo());
    widgetInfoList.append(d->propertyEditorView.widgetInfo());
#ifndef QTC_USE_QML_DESIGNER_LITE
    widgetInfoList.append(d->materialEditorView.widgetInfo());
    widgetInfoList.append(d->materialBrowserView.widgetInfo());
    widgetInfoList.append(d->textureEditorView.widgetInfo());
#endif
    widgetInfoList.append(d->statesEditorView.widgetInfo());

    if (checkEnterpriseLicense())
        widgetInfoList.append(d->contentLibraryView.widgetInfo());

    if (d->debugView.hasWidget())
        widgetInfoList.append(d->debugView.widgetInfo());

    for (auto &view : d->additionalViews) {
        if (view->hasWidget())
            widgetInfoList.append(view->widgetInfo());
    }

    return widgetInfoList;
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

void ViewManager::emitCustomNotification(const QString &identifier, const QList<ModelNode> &nodeList,
                                         const QList<QVariant> &data)
{
    d->nodeInstanceView.emitCustomNotification(identifier, nodeList, data);
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
    if (auto document = currentDesignDocument())
        return document->currentModel();

    return nullptr;
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

void ViewManager::jumpToCodeInTextEditor(const ModelNode &modelNode)
{
    d->textEditorView.action()->setChecked(true);
    ADS::DockWidget *dockWidget = qobject_cast<ADS::DockWidget *>(
        d->textEditorView.widgetInfo().widget->parentWidget());
    if (dockWidget)
        dockWidget->toggleView(true);
    d->textEditorView.jumpToModelNode(modelNode);
}

void ViewManager::addView(std::unique_ptr<AbstractView> &&view)
{
    d->additionalViews.push_back(std::move(view));
    registerViewAction(*d->additionalViews.back());
}

} // namespace QmlDesigner
