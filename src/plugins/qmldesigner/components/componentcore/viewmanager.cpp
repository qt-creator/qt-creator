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
#include <model/auxiliarypropertystorageview.h>
#include <navigatorview.h>
#include <nodeinstanceview.h>
#include <propertyeditorview.h>
#include <qmldesignerplugin.h>
#include <qmldesignertr.h>
#include <rewriterview.h>
#include <stateseditorview.h>
#include <texteditorview.h>

#include <qmldesignerbase/settings/designersettings.h>

#include <coreplugin/icore.h>

#include <sqlitedatabase.h>
#include <utils/algorithm.h>

#include <advanceddockingsystem/dockwidget.h>

#include <QElapsedTimer>
#include <QLoggingCategory>
#include <QTabWidget>

namespace QmlDesigner {

using ProjectManagerTracing::category;

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
        , assetsLibraryView{imageCache, externalDependencies}
        , itemLibraryView(imageCache, externalDependencies)
        , navigatorView{externalDependencies}
        , propertyEditorView(imageCache, externalDependencies)
#ifndef QTC_USE_QML_DESIGNER_LITE
        , materialBrowserView{imageCache, externalDependencies}
#endif
        , statesEditorView{externalDependencies}
    {}

    InteractiveConnectionManager connectionManager;
    CapturingConnectionManager capturingConnectionManager;
    QmlModelState savedState;
    Internal::DebugView debugView;
    Sqlite::Database auxiliaryDataDatabase{
        Utils::PathString{Core::ICore::userResourcePath("auxiliary_data.db").path()},
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
    MaterialBrowserView materialBrowserView;
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
    NanotraceHR::Tracer tracer{"view manager attach node instance view", category()};

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
    NanotraceHR::Tracer tracer{"view manager attach rewriter view", category()};

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
    NanotraceHR::Tracer tracer{"view manager detach rewriter view", category()};

    if (RewriterView *view = currentDesignDocument()->rewriterView()) {
        view->deactivateTextModifierChangeSignals();
        currentModel()->setRewriterView(nullptr);
    }
}

void ViewManager::switchStateEditorViewToBaseState()
{
    NanotraceHR::Tracer tracer{"view manager switch state editor view to base state", category()};

    if (d->statesEditorView.isAttached()) {
        d->savedState = d->statesEditorView.currentState();
        d->statesEditorView.setCurrentState(d->statesEditorView.baseState());
    }
}

void ViewManager::switchStateEditorViewToSavedState()
{
    NanotraceHR::Tracer tracer{"view manager switch state editor view to saved state", category()};

    if (d->savedState.isValid() && d->statesEditorView.isAttached())
        d->statesEditorView.setCurrentState(d->savedState);
}

QList<AbstractView *> ViewManager::views() const
{
    NanotraceHR::Tracer tracer{"view manager get views", category()};

    auto list = Utils::transform<QList<AbstractView *>>(d->additionalViews,
                                                        [](auto &&view) { return view.get(); });
    list.append(standardViews());
    return list;
}

void ViewManager::hideView(AbstractView &view)
{
    NanotraceHR::Tracer tracer{"view manager hide view", category()};

    disableView(view);
    view.setVisibility(false);
}

void ViewManager::showView(AbstractView &view)
{
    NanotraceHR::Tracer tracer{"view manager show view", category()};

    view.setVisibility(true);
    enableView(view);
}

QList<AbstractView *> ViewManager::standardViews() const
{
    NanotraceHR::Tracer tracer{"view manager get standard views", category()};

#ifndef QTC_USE_QML_DESIGNER_LITE
    QList<AbstractView *> list = {&d->auxiliaryDataKeyView,
                                  &d->edit3DView,
                                  &d->formEditorView,
                                  &d->textEditorView,
                                  &d->assetsLibraryView,
                                  &d->itemLibraryView,
                                  &d->navigatorView,
                                  &d->propertyEditorView,
                                  &d->materialBrowserView,
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
    NanotraceHR::Tracer tracer{"view manager register nanotrace actions", category()};

    if constexpr (isNanotraceEnabled()) {
        auto handleShutdownNanotraceAction = [](const SelectionContext &) {};
        auto shutdownNanotraceIcon = []() { return QIcon(); };
        auto startNanotraceAction = new ModelNodeAction("Start Nanotrace",
                                                        Tr::tr("Start Nanotrace"),
                                                        shutdownNanotraceIcon(),
                                                        Tr::tr("Start Nanotrace"),
                                                        ComponentCoreConstants::eventListCategory,
                                                        QKeySequence(),
                                                        22,
                                                        handleShutdownNanotraceAction);

        QObject::connect(startNanotraceAction->action(), &QAction::triggered, [&]() {
            d->nodeInstanceView.startNanotrace();
        });

        d->designerActionManagerView.designerActionManager().addDesignerAction(startNanotraceAction);

        auto shutDownNanotraceAction = new ModelNodeAction("ShutDown Nanotrace",
                                                           Tr::tr("Shut Down Nanotrace"),
                                                           shutdownNanotraceIcon(),
                                                           Tr::tr("Shut Down Nanotrace"),
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
    NanotraceHR::Tracer tracer{"view manager register view actions", category()};

    for (auto view : views()) {
        if (view->hasWidget())
            registerViewAction(*view);
    }
}

void ViewManager::registerViewAction(AbstractView &view)
{
    NanotraceHR::Tracer tracer{"view manager register view action", category()};

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
    NanotraceHR::Tracer tracer{"view manager enable view", category()};

    if (auto model = currentModel())
        model->attachView(&view);
}

void ViewManager::disableView(AbstractView &view)
{
    NanotraceHR::Tracer tracer{"view manager disable view", category()};

    if (auto model = currentModel())
        model->detachView(&view);
}

void ViewManager::resetPropertyEditorView()
{
    NanotraceHR::Tracer tracer{"view manager reset property editor view", category()};

    d->propertyEditorView.resetView();
}

void ViewManager::registerFormEditorTool(std::unique_ptr<AbstractCustomTool> &&tool)
{
    NanotraceHR::Tracer tracer{"view manager register form editor tool", category()};

    d->formEditorView.registerTool(std::move(tool));
}

void ViewManager::detachViewsExceptRewriterAndComponetView()
{
    NanotraceHR::Tracer tracer{"view manager detach views except rewriter and component view",
                               category()};

    switchStateEditorViewToBaseState();
    detachAdditionalViews();

    detachStandardViews();

    currentModel()->setNodeInstanceView(nullptr);
}

void ViewManager::attachAdditionalViews()
{
    NanotraceHR::Tracer tracer{"view manager attach additional views", category()};

    for (auto &view : d->additionalViews)
        currentModel()->attachView(view.get());
}

void ViewManager::detachAdditionalViews()
{
    NanotraceHR::Tracer tracer{"view manager detach additional views", category()};

    for (auto &view : d->additionalViews)
        currentModel()->detachView(view.get());
}

void ViewManager::detachStandardViews()
{
    NanotraceHR::Tracer tracer{"view manager detach standard views", category()};

    for (const auto &view : standardViews()) {
        if (view->isAttached())
            currentModel()->detachView(view);
    }
}

void ViewManager::attachComponentView()
{
    NanotraceHR::Tracer tracer{"view manager attach component view", category()};

    documentModel()->attachView(&d->componentView);
    QObject::connect(d->componentView.action(), &ComponentAction::currentComponentChanged,
                     currentDesignDocument(), &DesignDocument::changeToSubComponent);
    QObject::connect(d->componentView.action(), &ComponentAction::changedToMaster,
                     currentDesignDocument(), &DesignDocument::changeToMaster);
}

void ViewManager::detachComponentView()
{
    NanotraceHR::Tracer tracer{"view manager detach component view", category()};

    QObject::disconnect(d->componentView.action(), &ComponentAction::currentComponentChanged,
                        currentDesignDocument(), &DesignDocument::changeToSubComponent);
    QObject::disconnect(d->componentView.action(), &ComponentAction::changedToMaster,
                        currentDesignDocument(), &DesignDocument::changeToMaster);

    documentModel()->detachView(&d->componentView);
}

void ViewManager::attachViewsExceptRewriterAndComponetView()
{
    NanotraceHR::Tracer tracer{"view manager attach views except rewriter and component view",
                               category()};

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
    NanotraceHR::Tracer tracer{"view manager set component node", category()};

    d->componentView.setComponentNode(componentNode);
}

void ViewManager::setComponentViewToMaster()
{
    NanotraceHR::Tracer tracer{"view manager set component view to master", category()};

    d->componentView.setComponentToMaster();
}

void ViewManager::setNodeInstanceViewTarget(ProjectExplorer::Target *target)
{
    NanotraceHR::Tracer tracer{"view manager set node instance view target", category()};

    d->nodeInstanceView.setTarget(target);
}

void ViewManager::initializeWidgetInfos()
{
    NanotraceHR::Tracer tracer{"view manager initialize widget infos", category()};

    for (const auto &view : views()) {
        if (view->hasWidget()) {
            view->setWidgetRegistration(this);
            view->registerWidgetInfo();
        }
    }
}

QList<WidgetInfo> ViewManager::widgetInfos()
{
    NanotraceHR::Tracer tracer{"view manager get widget infos", category()};

    return m_widgetInfo;
}

void ViewManager::disableWidgets()
{
    NanotraceHR::Tracer tracer{"view manager disable widgets", category()};
    for (const auto &view : views())
        view->disableWidget();
}

void ViewManager::enableWidgets()
{
    NanotraceHR::Tracer tracer{"view manager enable widgets", category()};
    for (const auto &view : views())
        view->enableWidget();
}

void ViewManager::pushFileOnCrumbleBar(const Utils::FilePath &fileName)
{
    NanotraceHR::Tracer tracer{"view manager push file on crumble bar", category()};
    crumbleBar()->pushFile(fileName);
}

void ViewManager::pushInFileComponentOnCrumbleBar(const ModelNode &modelNode)
{
    NanotraceHR::Tracer tracer{"view manager push in-file component on crumble bar", category()};
    crumbleBar()->pushInFileComponent(modelNode);
}

void ViewManager::nextFileIsCalledInternally()
{
    NanotraceHR::Tracer tracer{"view manager next file is called internally", category()};
    crumbleBar()->nextFileIsCalledInternally();
}

const AbstractView *ViewManager::view() const
{
    return &d->nodeInstanceView;
}

AbstractView *ViewManager::view()
{
    return &d->nodeInstanceView;
}

TextEditorView *ViewManager::textEditorView()
{
    return &d->textEditorView;
}

void ViewManager::emitCustomNotification(const QString &identifier, const QList<ModelNode> &nodeList,
                                         const QList<QVariant> &data)
{
    NanotraceHR::Tracer tracer{"view manager emit custom notification", category()};

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
    NanotraceHR::Tracer tracer{"view manager qml js editor context help", category()};

    if (!view()->isAttached())
        callback({});

    ModelNode selectedNode = view()->firstSelectedModelNode();
    if (!selectedNode)
        selectedNode = view()->rootModelNode();

#ifdef QDS_USE_PROJECTSTORAGE
    Core::HelpItem helpItem("QML." + selectedNode.simplifiedTypeName(),
#else
    Core::HelpItem helpItem({QString::fromUtf8("QML." + selectedNode.type()),
                             "QML." + selectedNode.simplifiedTypeName()},
#endif
                            {},
                            {},
                            Core::HelpItem::QmlComponent);

    if (!helpItem.isValid()) {
        helpItem = Core::HelpItem(
            QUrl("qthelp://org.qt-project.qtdesignstudio/doc/quick-preset-components.html"));
    }

    callback(helpItem);
}

Model *ViewManager::currentModel() const
{
    NanotraceHR::Tracer tracer{"view manager get current model", category()};

    if (auto document = currentDesignDocument())
        return document->currentModel();

    return nullptr;
}

Model *ViewManager::documentModel() const
{
    NanotraceHR::Tracer tracer{"view manager get document model", category()};

    return currentDesignDocument()->documentModel();
}

void ViewManager::exportAsImage()
{
    NanotraceHR::Tracer tracer{"view manager export as image", category()};

    d->formEditorView.exportAsImage();
}

QImage ViewManager::takeFormEditorScreenshot()
{
    NanotraceHR::Tracer tracer{"view manager take form editor screenshot", category()};

    return d->formEditorView.takeFormEditorScreenshot();
}

void ViewManager::reformatFileUsingTextEditorView()
{
    NanotraceHR::Tracer tracer{"view manager reformat file using text editor view", category()};

    d->textEditorView.reformatFile();
}

bool ViewManager::usesRewriterView(RewriterView *rewriterView)
{
    NanotraceHR::Tracer tracer{"view manager uses rewriter view", category()};

    return currentDesignDocument()->rewriterView() == rewriterView;
}

void ViewManager::disableStandardViews()
{
    NanotraceHR::Tracer tracer{"view manager disable standard views", category()};

    d->disableStandardViews = true;
    detachStandardViews();
}

void ViewManager::enableStandardViews()
{
    NanotraceHR::Tracer tracer{"view manager enable standard views", category()};

    d->disableStandardViews = false;
    attachViewsExceptRewriterAndComponetView();
}

void ViewManager::jumpToCodeInTextEditor(const ModelNode &modelNode)
{
    NanotraceHR::Tracer tracer{"view manager jump to code in text editor", category()};

    d->textEditorView.action()->setChecked(true);
    ADS::DockWidget *dockWidget = qobject_cast<ADS::DockWidget *>(
        d->textEditorView.widgetInfo().widget->parentWidget());
    if (dockWidget)
        dockWidget->toggleView(true);
    d->textEditorView.jumpToModelNode(modelNode);
}

void ViewManager::addView(std::unique_ptr<AbstractView> &&view)
{
    NanotraceHR::Tracer tracer{"view manager add view", category()};

    d->additionalViews.push_back(std::move(view));
    registerViewAction(*d->additionalViews.back());
}

void ViewManager::registerWidgetInfo(WidgetInfo info)
{
    NanotraceHR::Tracer tracer{"view manager register widget info", category()};

    m_widgetInfo.append(info);
}

void ViewManager::deregisterWidgetInfo(WidgetInfo info)
{
    NanotraceHR::Tracer tracer{"view manager deregister widget info", category()};

    m_widgetInfo.removeIf(Utils::equal(&WidgetInfo::uniqueId, info.uniqueId));
}

} // namespace QmlDesigner
