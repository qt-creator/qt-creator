// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmldesignerplugin.h"
#include "qmldesignertr.h"

#include "coreplugin/iwizardfactory.h"
#include "designmodewidget.h"
#include "dynamiclicensecheck.h"
#include "exception.h"
#include "openuiqmlfiledialog.h"
#include "qmldesignerconstants.h"
#include "qmldesignerexternaldependencies.h"
#include "qmldesignerprojectmanager.h"
#include "quick2propertyeditorview.h"
#include "settingspage.h"
#include "shortcutmanager.h"
#include "toolbar.h"
#include "utils/checkablemessagebox.h"

#include <colortool/colortool.h>
#include <connectionview.h>
#include <curveeditor/curveeditorview.h>
#include <designeractionmanager.h>
#include <designsystemview/designsystemview.h>
#include <eventlist/eventlistpluginview.h>
#include <formeditor/view3dtool.h>
#include <studioquickwidget.h>
#include <devicesharing/devicemanager.h>
#include <pathtool/pathtool.h>
#include <qmljseditor/qmljseditor.h>
#include <qmljseditor/qmljseditorconstants.h>
#include <qmljseditor/qmljseditordocument.h>
#include <runmanager/runmanager.h>
#include <sourcetool/sourcetool.h>
#include <texttool/texttool.h>
#include <timelineeditor/timelineview.h>
#include <transitioneditor/transitioneditorview.h>

#include <qmljstools/qmljstoolsconstants.h>

#include <qmlprojectmanager/qmlproject.h>
#include <qmlprojectmanager/qmlprojectexporter/resourcegenerator.h>

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/coreplugintr.h>
#include <coreplugin/designmode.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/featureprovider.h>
#include <coreplugin/icore.h>
#include <coreplugin/idocument.h>
#include <coreplugin/iwizardfactory.h>
#include <coreplugin/messagebox.h>
#include <coreplugin/modemanager.h>
#include <extensionsystem/pluginmanager.h>
#include <extensionsystem/pluginspec.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/target.h>
#include <qmldesignerbase/qmldesignerbaseplugin.h>
#include <qmljs/qmljsmodelmanagerinterface.h>
#include <sqlite/sqlitelibraryinitializer.h>

#include <utils/algorithm.h>
#include <utils/guard.h>
#include <utils/hostosinfo.h>
#include <utils/mimeconstants.h>
#include <utils/qtcassert.h>
#include <utils/uniqueobjectptr.h>

#include <qplugin.h>
#include <QAction>
#include <QApplication>
#include <QDebug>
#include <QMessageBox>
#include <QProcessEnvironment>
#include <QQuickItem>
#include <QScreen>
#include <QTimer>
#include <QWindow>

#include <modelnodecontextmenu_helper.h>

static Q_LOGGING_CATEGORY(qmldesignerLog, "qtc.qmldesigner", QtWarningMsg)

using namespace Core;
using namespace QmlDesigner::Internal;

namespace QmlDesigner {

using ProjectManagingTracing::category;

namespace Internal {

class FullQDSFeatureProvider : public Core::IFeatureProvider
{
public:
    QSet<Utils::Id> availableFeatures(Utils::Id) const override
    {
        return {"QmlDesigner.Wizards.FullQDS"};
    }

    QSet<Utils::Id> availablePlatforms() const override { return {}; }

    QString displayNameForPlatform(Utils::Id) const override { return {}; }
};

class EnterpriseFeatureProvider : public Core::IFeatureProvider
{
public:
    QSet<Utils::Id> availableFeatures(Utils::Id) const override
    {
        return {"QmlDesigner.Wizards.Enterprise"};
    }
    QSet<Utils::Id> availablePlatforms() const override { return {}; }
    QString displayNameForPlatform(Utils::Id) const override { return {}; }
};

QString normalizeIdentifier(const QString &string)
{
    if (string.isEmpty())
        return {};
    QString ret = string;
    ret.remove(' ');
    ret[0] = ret.at(0).toLower();
    return ret;
}

class QtQuickDesignerFactory : public QmlJSEditor::QmlJSEditorFactory
{
public:
    QtQuickDesignerFactory();
};

QtQuickDesignerFactory::QtQuickDesignerFactory()
    : QmlJSEditorFactory(QmlJSEditor::Constants::C_QTQUICKDESIGNEREDITOR_ID)
{
    setDisplayName(Tr::tr("Qt Quick Designer"));

    addMimeType(Utils::Constants::QMLUI_MIMETYPE);
    setDocumentCreator([this]() {
        auto document = new QmlJSEditor::QmlJSEditorDocument(id());
        document->setIsDesignModePreferred(
                    QmlDesigner::QmlDesignerPlugin::settings().value(
                        QmlDesigner::DesignerSettingsKey::ALWAYS_DESIGN_MODE).toBool());
        return document;
    });
}

} // namespace Internal

struct TraceIdentifierData
{
    TraceIdentifierData(const QString &_identifier, const QString &_newIdentifer, int _duration)
        : identifier(_identifier), newIdentifer(_newIdentifer), maxDuration(_duration)
    {}

    TraceIdentifierData() = default;

    QString identifier;
    QString newIdentifer;
    int maxDuration;
    int time = 0;
};

class QmlDesignerPluginPrivate
{
public:
    QmlDesignerPluginPrivate()
        : projectManager{viewManager, documentManager}
    {}

public:
    ExternalDependencies externalDependencies{QmlDesignerBasePlugin::settings()};
    QmlDesignerProjectManager projectManager;
    ViewManager viewManager{projectManager.asynchronousImageCache(),
                            externalDependencies,
                            projectManager.modulesStorage()};
    DocumentManager documentManager{projectManager, externalDependencies};
    ShortCutManager shortCutManager;
    DeviceShare::DeviceManager deviceManager;
    RunManager runManager{deviceManager};
    SettingsPage settingsPage;
    DesignModeWidget mainWidget;
    QtQuickDesignerFactory m_qtQuickDesignerFactory;
    Utils::UniqueObjectPtr<QToolBar> toolBar;
    Utils::UniqueObjectPtr<QWidget> statusBar;
    QHash<QString, TraceIdentifierData> m_traceIdentifierDataHash;
    QHash<QString, TraceIdentifierData> m_activeTraceIdentifierDataHash;
    QElapsedTimer timer;
};

QmlDesignerPlugin *QmlDesignerPlugin::m_instance = nullptr;

static bool isInDesignerMode()
{
    return Core::ModeManager::currentModeId() == Core::Constants::MODE_DESIGN;
}

static bool checkIfEditorIsQtQuick(Core::IEditor *editor)
{
    if (editor
        && (editor->document()->id() == QmlJSEditor::Constants::C_QMLJSEDITOR_ID
            || editor->document()->id() == QmlJSEditor::Constants::C_QTQUICKDESIGNEREDITOR_ID)) {
        QmlJS::ModelManagerInterface *modelManager = QmlJS::ModelManagerInterface::instance();
        QmlJS::Document::Ptr document = modelManager->ensuredGetDocumentForPath(
            editor->document()->filePath());
        if (!document.isNull())
            return document->language() == QmlJS::Dialect::QmlQtQuick2
                    || document->language() == QmlJS::Dialect::QmlQtQuick2Ui
                    || document->language() == QmlJS::Dialect::Qml;

        if (Core::ModeManager::currentModeId() == Core::Constants::MODE_DESIGN) {
            Core::AsynchronousMessageBox::warning(
                Tr::tr("Cannot Open Design Mode"),
                Tr::tr("The QML file is not currently opened in a QML Editor."));
            Core::ModeManager::activateMode(Core::Constants::MODE_EDIT);
        }
    }

    return false;
}

static bool isDesignerMode(Utils::Id mode)
{
    return mode == Core::Constants::MODE_DESIGN;
}

static bool documentIsAlreadyOpen(DesignDocument *designDocument, Core::IEditor *editor, Utils::Id newMode)
{
    return designDocument
           && editor == designDocument->editor()
           && isDesignerMode(newMode)
           && designDocument->fileName() == editor->document()->filePath();
}

static bool warningsForQmlFilesInsteadOfUiQmlEnabled()
{
    return QmlDesignerPlugin::settings().value(DesignerSettingsKey::WARNING_FOR_QML_FILES_INSTEAD_OF_UIQML_FILES).toBool();
}

QmlDesignerPlugin::QmlDesignerPlugin()
{
    NanotraceHR::Tracer tracer{"qml designer plugin constructor", category()};

    m_instance = this;
}

QmlDesignerPlugin::~QmlDesignerPlugin()
{
    NanotraceHR::Tracer tracer{"qml designer plugin destructor", category()};

    if (d)
        Core::DesignMode::unregisterDesignWidget(&d->mainWidget);
    delete d;
    d = nullptr;
    m_instance = nullptr;
}

////////////////////////////////////////////////////
//
// INHERITED FROM ExtensionSystem::Plugin
//
////////////////////////////////////////////////////
Utils::Result<> QmlDesignerPlugin::initialize(const QStringList &)
{
    NanotraceHR::Tracer tracer{"qml designer plugin initialize", category()};

    if constexpr (isUsingQmlDesignerLite()) {
        if (!QmlDesignerBasePlugin::isLiteModeEnabled()) {
            QMessageBox::warning(Core::ICore::dialogParent(),
                                 tr("Qml Designer Lite"),
                                 tr("The Qml Designer Lite plugin is not enabled."));
            return Utils::ResultError(tr("Qml Designer Lite initialization error: "
                                         "The Qml Designer Lite plugin is not enabled."));
        }
    }

    Sqlite::LibraryInitializer::initialize();
    QDir{}.mkpath(Core::ICore::cacheResourcePath().toUrlishString());

    if (Core::ICore::isQtDesignStudio()) {
        QAction *action = new QAction(tr("Give Feedback..."), this);
        action->setVisible(false); // keep hidden unless UsageStatistic plugin activates it
        Core::Command *cmd = Core::ActionManager::registerAction(action, "Help.GiveFeedback");
        Core::ActionManager::actionContainer(Core::Constants::M_HELP)
            ->addAction(cmd, Core::Constants::G_HELP_SUPPORT);

        connect(action, &QAction::triggered, this, [this] {
            launchFeedbackPopupInternal(QGuiApplication::applicationDisplayName());
        });
    }

    d = new QmlDesignerPluginPrivate;
    d->timer.start();
    if (Core::ICore::isQtDesignStudio())
        QmlProjectManager::QmlProjectExporter::ResourceGenerator::generateMenuEntry(this);

    const QString fontPath
        = Core::ICore::resourcePath(
                "qmldesigner/propertyEditorQmlSources/imports/StudioTheme/icons.ttf")
              .toUrlishString();
    if (QFontDatabase::addApplicationFont(fontPath) < 0)
        qCWarning(qmldesignerLog) << "Could not add font " << fontPath << "to font database";

    //TODO Move registering those types out of the property editor, since they are used also in the states editor
    Quick2PropertyEditorView::registerQmlTypes();
    StudioQuickWidget::registerDeclarativeType();

    Exception::setWarnAboutException(!QmlDesignerPlugin::instance()
                                          ->settings()
                                          .value(DesignerSettingsKey::ENABLE_MODEL_EXCEPTION_OUTPUT)
                                          .toBool());

    Exception::setShowExceptionCallback([&](QStringView title, QStringView description) {
        const QString composedTitle = title.isEmpty() ? Tr::tr("Error") : title.toString();
        Core::AsynchronousMessageBox::warning(composedTitle, description.toString());
    });

    if (Core::ICore::isQtDesignStudio()) {
        d->toolBar = ToolBar::create();
        d->statusBar = ToolBar::createStatusBar();
    }

    return Utils::ResultOk;
}

bool QmlDesignerPlugin::delayedInitialize()
{
    NanotraceHR::Tracer tracer{"qml designer plugin delayed initialize", category()};

    enforceDelayedInitialize();
    return true;
}

void QmlDesignerPlugin::extensionsInitialized()
{
    NanotraceHR::Tracer tracer{"qml designer plugin extensions initialized", category()};

    Core::DesignMode::setDesignModeIsRequired();
    // delay after Core plugin's extensionsInitialized, so the DesignMode is availabe
    connect(Core::ICore::instance(), &Core::ICore::coreAboutToOpen, this, [this] {
        integrateIntoQtCreator(&d->mainWidget);
    });

    auto &actionManager = d->viewManager.designerActionManager();
    actionManager.createDefaultDesignerActions();
    actionManager.createDefaultAddResourceHandler();
    actionManager.createDefaultModelNodePreviewImageHandlers();
    actionManager.polishActions();

    registerCombinedTracedPoints(Constants::EVENT_STATE_ADDED,
                                 Constants::EVENT_STATE_CLONED,
                                 Constants::EVENT_STATE_ADDED_AND_CLONED);

    if (checkEnterpriseLicense())
        Core::IWizardFactory::registerFeatureProvider(new EnterpriseFeatureProvider);

    if (!QmlDesignerBasePlugin::isLiteModeEnabled())
        Core::IWizardFactory::registerFeatureProvider(new FullQDSFeatureProvider);
}

ExtensionSystem::IPlugin::ShutdownFlag QmlDesignerPlugin::aboutToShutdown()
{
    NanotraceHR::Tracer tracer{"qml designer plugin about to shutdown", category()};

    Utils::QtcSettings *settings = Core::ICore::settings();

    if (!Utils::CheckableDecider("FeedbackPopup").shouldAskAgain())
        return SynchronousShutdown;

    int shutdownCount = settings->value("ShutdownCount", 0).toInt();
    settings->setValue("ShutdownCount", ++shutdownCount);

    if (!settings->value("UsageStatistic/TrackingEnabled").toBool())
        return SynchronousShutdown;

    if (shutdownCount >= 5) {
        m_shutdownPending = true;
        launchFeedbackPopupInternal(QGuiApplication::applicationDisplayName());
        return AsynchronousShutdown;
    }

    return SynchronousShutdown;
}

static QStringList allUiQmlFilesforCurrentProject(const Utils::FilePath &fileName)
{
    QStringList list;
    ProjectExplorer::Project *currentProject = ProjectExplorer::ProjectManager::projectForFile(fileName);

    if (currentProject) {
        const QList<Utils::FilePath> fileNames = currentProject->files(ProjectExplorer::Project::SourceFiles);
        for (const Utils::FilePath &fileName : fileNames) {
            if (fileName.endsWith(".ui.qml"))
                list.append(fileName.toUrlishString());
        }
    }

    return list;
}

static QString projectPath(const Utils::FilePath &fileName)
{
    QString path;
    ProjectExplorer::Project *currentProject = ProjectExplorer::ProjectManager::projectForFile(fileName);

    if (currentProject)
        path = currentProject->projectDirectory().toUrlishString();

    return path;
}

void QmlDesignerPlugin::integrateIntoQtCreator(DesignModeWidget *modeWidget)
{
    NanotraceHR::Tracer tracer{"qml designer plugin integrate into Qt Creator", category()};

    const Context context(Constants::qmlDesignerContextId, Constants::qtQuickToolsMenuContextId);
    IContext::attach(modeWidget, context, [modeWidget](const IContext::HelpCallback &callback) {
        modeWidget->contextHelp(callback);
    });

    Core::Context qmlDesignerMainContext(Constants::qmlDesignerContextId);
    Core::Context qmlDesignerFormEditorContext(Constants::qmlFormEditorContextId);
    Core::Context qmlDesignerEditor3dContext(Constants::qml3DEditorContextId);
    Core::Context qmlDesignerNavigatorContext(Constants::qmlNavigatorContextId);
    Core::Context qmlDesignerMaterialBrowserContext(Constants::qmlMaterialBrowserContextId);
    Core::Context qmlDesignerAssetsLibraryContext(Constants::qmlAssetsLibraryContextId);

    d->shortCutManager.registerActions(qmlDesignerMainContext, qmlDesignerFormEditorContext,
                                       qmlDesignerEditor3dContext, qmlDesignerNavigatorContext);

    const QStringList mimeTypes = { Utils::Constants::QML_MIMETYPE,
                                    Utils::Constants::QMLUI_MIMETYPE };

    Core::DesignMode::registerDesignWidget(modeWidget, mimeTypes, context);

    connect(Core::DesignMode::instance(), &Core::DesignMode::actionsUpdated,
        &d->shortCutManager, &ShortCutManager::updateActions);

    connect(Core::EditorManager::instance(), &Core::EditorManager::currentEditorChanged, [this] (Core::IEditor *editor) {
        if (d && checkIfEditorIsQtQuick(editor) && isInDesignerMode())
            changeEditor();
    });

    connect(Core::EditorManager::instance(), &Core::EditorManager::editorsClosed, [this] (QList<Core::IEditor*> editors) {
        if (d) {
            if (d->documentManager.hasCurrentDesignDocument()
                    && editors.contains(currentDesignDocument()->textEditor()))
                hideDesigner();

            d->documentManager.removeEditors(editors);
        }
    });

    connect(Core::ModeManager::instance(),
            &Core::ModeManager::currentModeChanged,
            [this](Utils::Id newMode, Utils::Id oldMode) {
                Core::IEditor *currentEditor = Core::EditorManager::currentEditor();
                if (isDesignerMode(newMode) && checkIfEditorIsQtQuick(currentEditor)
                    && !documentIsAlreadyOpen(currentDesignDocument(), currentEditor, newMode)) {
                    showDesigner();
                } else if (currentDesignDocument()
                           || (!isDesignerMode(newMode) && isDesignerMode(oldMode))) {
                    hideDesigner();
                }
            });
}

void QmlDesignerPlugin::clearDesigner()
{
    NanotraceHR::Tracer tracer{"qml designer plugin clear designer", category()};

    if (d->documentManager.hasCurrentDesignDocument()) {
        deactivateAutoSynchronization();
        d->mainWidget.saveSettings();
    }
}

void QmlDesignerPlugin::resetDesignerDocument()
{
    NanotraceHR::Tracer tracer{"qml designer plugin reset designer document", category()};

    d->shortCutManager.disconnectUndoActions(currentDesignDocument());
    d->documentManager.setCurrentDesignDocument(nullptr);
    d->shortCutManager.updateActions(nullptr);
    d->shortCutManager.updateUndoActions(nullptr);
}

void QmlDesignerPlugin::setupDesigner()
{
    NanotraceHR::Tracer tracer{"qml designer plugin setup designer", category()};

    auto currentEditor = Core::EditorManager::currentEditor();
    d->projectManager.updateIfFilesListInProjectIsChanged(currentEditor);
    d->shortCutManager.disconnectUndoActions(currentDesignDocument());
    d->documentManager.setCurrentDesignDocument(currentEditor);
    d->shortCutManager.connectUndoActions(currentDesignDocument());

    if (d->documentManager.hasCurrentDesignDocument()) {
        activateAutoSynchronization();
        d->shortCutManager.updateActions(currentDesignDocument()->textEditor());
        d->viewManager.pushFileOnCrumbleBar(currentDesignDocument()->fileName());
        d->viewManager.setComponentViewToMaster();
    }

    d->shortCutManager.updateUndoActions(currentDesignDocument());
}

static bool checkUiQMLNagScreen(const Utils::FilePath &fileName)
{
    const QStringList allUiQmlFiles = allUiQmlFilesforCurrentProject(fileName);
    static bool doOnce = true;
    if (doOnce && warningsForQmlFilesInsteadOfUiQmlEnabled() && !fileName.endsWith(".ui.qml")
        && !allUiQmlFiles.isEmpty()) {
        OpenUiQmlFileDialog dialog(Core::ICore::dialogParent());
        dialog.setUiQmlFiles(projectPath(fileName), allUiQmlFiles);
        dialog.exec();
        if (dialog.uiFileOpened()) {
            Core::ModeManager::activateMode(Core::Constants::MODE_EDIT);
            Core::EditorManager::openEditorAt({Utils::FilePath::fromString(dialog.uiQmlFile()), 0, 0});
            return true;
        }
        doOnce = false;
    }

    return false;
}

void QmlDesignerPlugin::showDesigner()
{
    NanotraceHR::Tracer tracer{"qml designer plugin show designer", category()};

    QTC_ASSERT(!d->documentManager.hasCurrentDesignDocument(), return);

    enforceDelayedInitialize();

    d->mainWidget.initialize();

    if (checkUiQMLNagScreen(Core::EditorManager::currentEditor()->document()->filePath()))
        return;

    setupDesigner();

    m_usageTimer.restart();
}

void QmlDesignerPlugin::hideDesigner()
{
    NanotraceHR::Tracer tracer{"qml designer plugin hide designer", category()};

    clearDesigner();
    resetDesignerDocument();
}

void QmlDesignerPlugin::changeEditor()
{
    NanotraceHR::Tracer tracer{"qml designer plugin change editor", category()};

    if (d->mainWidget.isInitialized()) {
        // showDesigner was already already called
        clearDesigner();
        setupDesigner();
    } else {
        // we are already in Design mode, but showDesigner wasn't called yet,
        // so we need to call that to set up the widgets
        showDesigner();
    }
}

void QmlDesignerPlugin::jumpTextCursorToSelectedModelNode()
{
    NanotraceHR::Tracer tracer{"qml designer plugin jump text cursor to selected model node",
                               category()};

    // visual editor -> text editor
    ModelNode selectedNode;
    if (!rewriterView()->selectedModelNodes().isEmpty())
        selectedNode = rewriterView()->selectedModelNodes().constFirst();

    if (selectedNode.isValid()) {
        const int nodeOffset = rewriterView()->nodeOffset(selectedNode);
        if (nodeOffset > 0) {
            const ModelNode currentSelectedNode = rewriterView()->
                nodeAtTextCursorPosition(currentDesignDocument()->plainTextEdit()->textCursor().position());
            if (currentSelectedNode != selectedNode) {
                int line, column;
                currentDesignDocument()->textEditor()->convertPosition(nodeOffset, &line, &column);
                // line has to be 1 based, column 0 based!
                currentDesignDocument()->textEditor()->gotoLine(line, column - 1);
            }
        }
    }
}

void QmlDesignerPlugin::selectModelNodeUnderTextCursor()
{
    NanotraceHR::Tracer tracer{"qml designer plugin select model node under text cursor", category()};

    const int cursorPosition = currentDesignDocument()->plainTextEdit()->textCursor().position();
    ModelNode modelNode = rewriterView()->nodeAtTextCursorPosition(cursorPosition);
    if (modelNode.isValid())
        rewriterView()->setSelectedModelNode(modelNode);
}

void QmlDesignerPlugin::activateAutoSynchronization()
{
    NanotraceHR::Tracer tracer{"qml designer plugin activate auto synchronization", category()};

    viewManager().detachViewsExceptRewriterAndComponetView();
    viewManager().detachComponentView();

    // text editor -> visual editor
    if (!currentDesignDocument()->isDocumentLoaded())
        currentDesignDocument()->loadDocument(currentDesignDocument()->plainTextEdit());

    currentDesignDocument()->updateActiveTarget();
    d->mainWidget.enableWidgets();
    currentDesignDocument()->attachRewriterToModel();

    resetModelSelection();

    viewManager().attachComponentView();
    viewManager().attachViewsExceptRewriterAndComponetView();

    selectModelNodeUnderTextCursor();

    d->mainWidget.setupNavigatorHistory(currentDesignDocument()->textEditor());
}

void QmlDesignerPlugin::deactivateAutoSynchronization()
{
    NanotraceHR::Tracer tracer{"qml designer plugin deactivate auto synchronization", category()};

    viewManager().detachViewsExceptRewriterAndComponetView();
    viewManager().detachComponentView();
    viewManager().detachRewriterView();
    documentManager().currentDesignDocument()->resetToDocumentModel();
}

void QmlDesignerPlugin::resetModelSelection()
{
    NanotraceHR::Tracer tracer{"qml designer plugin reset model selection", category()};

    if (!rewriterView()) {
        qCWarning(qmldesignerLog) << "No rewriter existing while calling resetModelSelection";
        return;
    }
    if (!currentModel()) {
        qCWarning(qmldesignerLog) << "No current QmlDesigner document model while calling resetModelSelection";
        return;
    }
    rewriterView()->setSelectedModelNodes(QList<ModelNode>());
}

QString QmlDesignerPlugin::identiferToDisplayString(const QString &identifier)
{
    NanotraceHR::Tracer tracer{"qml designer plugin identifier to display string", category()};

    for (AbstractView *view : viewManager().views())
        if (view->widgetInfo().uniqueId.toLower() == identifier.toLower())
            return view->widgetInfo().feedbackDisplayName;

    return identifier;
}

RewriterView *QmlDesignerPlugin::rewriterView() const
{
    NanotraceHR::Tracer tracer{"qml designer plugin rewriter view", category()};

    return currentDesignDocument()->rewriterView();
}

Model *QmlDesignerPlugin::currentModel() const
{
    NanotraceHR::Tracer tracer{"qml designer plugin current model", category()};

    return currentDesignDocument()->currentModel();
}

QmlDesignerPluginPrivate *QmlDesignerPlugin::privateInstance()
{
    NanotraceHR::Tracer tracer{"qml designer plugin private instance", category()};

    QTC_ASSERT(instance(), return nullptr);
    return instance()->d;
}

void QmlDesignerPlugin::enforceDelayedInitialize()
{
    NanotraceHR::Tracer tracer{"qml designer plugin enforce delayed initialize", category()};

    if (m_delayedInitialized)
        return;

    // adding default path to item library plugins
    const QString postfix = Utils::HostOsInfo::isMacHost()
                                ? QString("QmlDesigner")
                                : QString("qmldesigner");
    const QStringList pluginPaths =
        Utils::transform(ExtensionSystem::PluginManager::pluginPaths(),
                         [postfix](const Utils::FilePath &p) {
                           return (p / postfix).toFSPathString();
                         });

    d->viewManager.registerView(std::make_unique<ConnectionView>(d->externalDependencies));

    auto timelineView = d->viewManager.registerView(
        std::make_unique<TimelineView>(d->externalDependencies, d->projectManager.modulesStorage()));
    timelineView->registerActions();

    d->viewManager.registerView(std::make_unique<CurveEditorView>(d->externalDependencies));

    auto eventlistView = d->viewManager.registerView(
        std::make_unique<EventListPluginView>(d->externalDependencies,
                                              d->projectManager.modulesStorage()));
    eventlistView->registerActions();

    auto transitionEditorView = d->viewManager.registerView(
        std::make_unique<TransitionEditorView>(d->externalDependencies));
    transitionEditorView->registerActions();

    if (QmlDesignerBasePlugin::experimentalFeaturesEnabled())
        d->viewManager.registerView(std::make_unique<DesignSystemView>(d->externalDependencies));

    d->viewManager.registerFormEditorTool(std::make_unique<SourceTool>());
    d->viewManager.registerFormEditorTool(std::make_unique<ColorTool>());
    d->viewManager.registerFormEditorTool(std::make_unique<TextTool>());
    d->viewManager.registerFormEditorTool(std::make_unique<PathTool>(d->externalDependencies));
    d->viewManager.registerFormEditorTool(std::make_unique<View3DTool>());

    if (Core::ICore::isQtDesignStudio()) {
        d->mainWidget.initialize();

        if (QmlProjectManager::QmlProject::isQtDesignStudioStartedFromQtC())
            emitUsageStatistics("QDSlaunchedFromQtC");

        FoundLicense license = checkLicense();
        if (license == FoundLicense::enterprise)
            Core::ICore::setPrependAboutInformation("License: Enterprise");
        else if (license == FoundLicense::professional)
            Core::ICore::setPrependAboutInformation("License: Professional");
        else if (license == FoundLicense::community)
            Core::ICore::setPrependAboutInformation("License: Community");
    }

    m_delayedInitialized = true;
}

DesignDocument *QmlDesignerPlugin::currentDesignDocument() const
{
    NanotraceHR::Tracer tracer{"qml designer plugin current design document", category()};

    return d ? d->documentManager.currentDesignDocument() : nullptr;
}

Internal::DesignModeWidget *QmlDesignerPlugin::mainWidget() const
{
    NanotraceHR::Tracer tracer{"qml designer plugin main widget", category()};

    return d ? &d->mainWidget : nullptr;
}

QmlDesignerProjectManager &QmlDesignerPlugin::projectManagerForPluginInitializationOnly()
{
    NanotraceHR::Tracer tracer{"qml designer plugin project manager for plugin initialization only",
                               category()};

    return m_instance->d->projectManager;
}

QWidget *QmlDesignerPlugin::createProjectExplorerWidget(QWidget *parent) const
{
    NanotraceHR::Tracer tracer{"qml designer plugin create project explorer widget", category()};

    return Internal::DesignModeWidget::createProjectExplorerWidget(parent);
}

void QmlDesignerPlugin::switchToTextModeDeferred()
{
    NanotraceHR::Tracer tracer{"qml designer plugin switch to text mode deferred", category()};

    QTimer::singleShot(0, this, [] { Core::ModeManager::activateMode(Core::Constants::MODE_EDIT); });
}

double QmlDesignerPlugin::formEditorDevicePixelRatio()
{
    NanotraceHR::Tracer tracer{"qml designer plugin form editor device pixel ratio", category()};

    if (QmlDesignerPlugin::settings().value(DesignerSettingsKey::IGNORE_DEVICE_PIXEL_RATIO).toBool())
        return 1;

    const QList<QWindow *> topLevelWindows = QApplication::topLevelWindows();
    if (topLevelWindows.isEmpty())
        return 1;
    return topLevelWindows.constFirst()->screen()->devicePixelRatio();
}

void QmlDesignerPlugin::contextHelp(const Core::IContext::HelpCallback &callback, const QString &id)
{
    NanotraceHR::Tracer tracer{"qml designer plugin context help", category()};

    emitUsageStatistics(Constants::EVENT_HELP_REQUESTED + id);
    QmlDesignerPlugin::instance()->viewManager().qmlJSEditorContextHelp(callback);
}

void QmlDesignerPlugin::emitUsageStatistics(const QString &identifier)
{
    NanotraceHR::Tracer tracer{"qml designer plugin emit usage statistics", category()};

    QTC_ASSERT(instance(), return);
    emit instance()->usageStatisticsNotifier(normalizeIdentifier(identifier));

    TraceIdentifierData activeData = privateInstance()->m_activeTraceIdentifierDataHash.value(
        identifier);

    if (activeData.time) {
        const int currentTime = privateInstance()->timer.elapsed();
        const int currentDuration = (currentTime - activeData.time);
        if (currentDuration < activeData.maxDuration)
            emit instance()->usageStatisticsUsageDuration(activeData.newIdentifer, currentDuration);

        privateInstance()->m_activeTraceIdentifierDataHash.remove(identifier);
    }

    TraceIdentifierData data = privateInstance()->m_traceIdentifierDataHash.value(identifier);

    if (!data.identifier.isEmpty()) {
        data.time = privateInstance()->timer.elapsed();
        privateInstance()->m_activeTraceIdentifierDataHash.insert(data.identifier, data);
    }

    const auto values = privateInstance()->m_activeTraceIdentifierDataHash.values();
    for (const auto &activeData : values) {
        const int currentTime = privateInstance()->timer.elapsed();
        const int currentDuration = (currentTime - activeData.time);

        if (currentDuration > activeData.maxDuration) {
            privateInstance()->m_activeTraceIdentifierDataHash.remove(activeData.identifier);
        }
    }
}

void QmlDesignerPlugin::emitUsageStatisticsContextAction(const QString &identifier)
{
    NanotraceHR::Tracer tracer{"qml designer plugin emit usage statistics context action", category()};

    emitUsageStatistics(Constants::EVENT_ACTION_EXECUTED + identifier);
}

AsynchronousImageCache &QmlDesignerPlugin::imageCache()
{
    NanotraceHR::Tracer tracer{"qml designer plugin image cache", category()};

    return m_instance->d->projectManager.asynchronousImageCache();
}

void QmlDesignerPlugin::registerPreviewImageProvider(QQmlEngine *engine)
{
    NanotraceHR::Tracer tracer{"qml designer plugin register preview image provider", category()};

    m_instance->d->projectManager.registerPreviewImageProvider(engine);
}

void QmlDesignerPlugin::trackWidgetFocusTime(QWidget *widget, const QString &identifier)
{
    NanotraceHR::Tracer tracer{"qml designer plugin track widget focus time", category()};

    connect(qApp, &QApplication::focusChanged, widget, [widget, identifier](QWidget *from, QWidget *to) {
        static QElapsedTimer widgetUsageTimer;
        static QString lastIdentifier;
        if (widget->isAncestorOf(to)) {
            if (!lastIdentifier.isEmpty())
                emitUsageStatisticsTime(lastIdentifier, widgetUsageTimer.elapsed());
            widgetUsageTimer.restart();
            lastIdentifier = identifier;
        } else if (widget->isAncestorOf(from) && lastIdentifier == identifier) {
            emitUsageStatisticsTime(identifier, widgetUsageTimer.elapsed());
            lastIdentifier.clear();
        }
    });
}

void QmlDesignerPlugin::registerCombinedTracedPoints(const QString &identifierFirst,
                                                     const QString &identifierSecond,
                                                     const QString &newIdentifier,
                                                     int maxDuration)
{
    NanotraceHR::Tracer tracer{"qml designer plugin register combined traced points", category()};

    QTC_ASSERT(privateInstance(), return);
    privateInstance()->m_traceIdentifierDataHash.insert(identifierFirst,
                                                        TraceIdentifierData(identifierSecond,
                                                                            newIdentifier,
                                                                            maxDuration));
}

void QmlDesignerPlugin::launchFeedbackPopup(const QString &identifier)
{
    NanotraceHR::Tracer tracer{"qml designer plugin launch feedback popup", category()};

    if (Core::ModeManager::currentModeId() == Core::Constants::MODE_DESIGN)
        launchFeedbackPopupInternal(identifier);
}

void QmlDesignerPlugin::handleFeedback(const QString &feedback, int rating)
{
    NanotraceHR::Tracer tracer{"qml designer plugin handle feedback", category()};

    const QString identifier = sender()->property("identifier").toString();
    emit usageStatisticsInsertFeedback(identifier, feedback, rating);
}

void QmlDesignerPlugin::launchFeedbackPopupInternal(const QString &identifier)
{
    NanotraceHR::Tracer tracer{"qml designer plugin launch feedback popup internal", category()};

    m_feedbackWidget = new QQuickWidget(Core::ICore::dialogParent());
    m_feedbackWidget->setObjectName(Constants::OBJECT_NAME_TOP_FEEDBACK);

    const QString qmlPath = Core::ICore::resourcePath("qmldesigner/feedback/FeedbackPopup.qml").toUrlishString();

    m_feedbackWidget->setSource(QUrl::fromLocalFile(qmlPath));
    if (Utils::HostOsInfo::isLinuxHost()) {
        QPoint pos = Core::ICore::dialogParent()->pos();
        int x = (Core::ICore::dialogParent()->width() - m_feedbackWidget->width()) / 2;
        int y = (Core::ICore::dialogParent()->height() - m_feedbackWidget->height()) / 2;
        m_feedbackWidget->move(pos.x() + x, pos.y() + y);
    }
    if (!m_feedbackWidget->errors().isEmpty()) {
        qDebug() << qmlPath;
        qDebug() << m_feedbackWidget->errors().first().toString();
    }
    m_feedbackWidget->setWindowModality(Qt::ApplicationModal);
    if (Utils::HostOsInfo::isMacHost())
        m_feedbackWidget->setWindowFlags(Qt::Dialog);
    else
        m_feedbackWidget->setWindowFlags(Qt::SplashScreen);
    m_feedbackWidget->setAttribute(Qt::WA_DeleteOnClose);

    QQuickItem *root = m_feedbackWidget->rootObject();

    QTC_ASSERT(root, return );

    QObject *title = root->findChild<QObject *>("title");
    QString name = Tr::tr("Enjoying %1?").arg(identiferToDisplayString(identifier));
    title->setProperty("text", name);
    root->setProperty("identifier", identifier);

    connect(root, SIGNAL(closeClicked()), this, SLOT(closeFeedbackPopup()));

    QObject::connect(root,
                     SIGNAL(submitFeedback(QString, int)),
                     this,
                     SLOT(handleFeedback(QString, int)));

    m_feedbackWidget->show();
}

void QmlDesignerPlugin::closeFeedbackPopup()
{
    NanotraceHR::Tracer tracer{"qml designer plugin close feedback popup", category()};
    if (m_feedbackWidget) {
        m_feedbackWidget->deleteLater();
        m_feedbackWidget = nullptr;
    }

    if (m_shutdownPending) {
        Utils::CheckableDecider("FeedbackPopup").doNotAskAgain();
        emit asynchronousShutdownFinished();
    }
}

void QmlDesignerPlugin::emitUsageStatisticsTime(const QString &identifier, int elapsed)
{
    NanotraceHR::Tracer tracer{"qml designer plugin emit usage statistics time", category()};

    QTC_ASSERT(instance(), return);
    emit instance()->usageStatisticsUsageTimer(normalizeIdentifier(identifier), elapsed);
}

void QmlDesignerPlugin::emitUsageStatisticsUsageDuration(const QString &identifier, int elapsed)
{
    NanotraceHR::Tracer tracer{"qml designer plugin emit usage statistics usage duration", category()};

    QTC_ASSERT(instance(), return);
    emit instance()->usageStatisticsUsageDuration(identifier, elapsed);
}

QmlDesignerPlugin *QmlDesignerPlugin::instance()
{
    NanotraceHR::Tracer tracer{"qml designer plugin instance", category()};

    return m_instance;
}

DocumentManager &QmlDesignerPlugin::documentManager()
{
    NanotraceHR::Tracer tracer{"qml designer plugin document manager", category()};

    return d->documentManager;
}

const DocumentManager &QmlDesignerPlugin::documentManager() const
{
    NanotraceHR::Tracer tracer{"qml designer plugin document manager const", category()};

    return d->documentManager;
}

ViewManager &QmlDesignerPlugin::viewManager()
{
    NanotraceHR::Tracer tracer{"qml designer plugin view manager", category()};

    return instance()->d->viewManager;
}

DeviceShare::DeviceManager &QmlDesignerPlugin::deviceManager()
{
    NanotraceHR::Tracer tracer{"qml designer plugin device manager", category()};

    return instance()->d->deviceManager;
}

RunManager &QmlDesignerPlugin::runManager()
{
    NanotraceHR::Tracer tracer{"qml designer plugin run manager", category()};

    return instance()->d->runManager;
}

DesignerActionManager &QmlDesignerPlugin::designerActionManager()
{
    NanotraceHR::Tracer tracer{"qml designer plugin designer action manager", category()};

    return d->viewManager.designerActionManager();
}

const DesignerActionManager &QmlDesignerPlugin::designerActionManager() const
{
    NanotraceHR::Tracer tracer{"qml designer plugin designer action manager const", category()};

    return d->viewManager.designerActionManager();
}

ExternalDependenciesInterface &QmlDesignerPlugin::externalDependenciesForPluginInitializationOnly()
{
    NanotraceHR::Tracer tracer{
        "qml designer plugin external dependencies for plugin initialization only", category()};

    return instance()->d->externalDependencies;
}

ADS::DockManager *QmlDesignerPlugin::dockManagerForPluginInitializationOnly()
{
    NanotraceHR::Tracer tracer{"qml designer plugin dock manager", category()};

    return m_instance->d->mainWidget.dockManager();
}

DesignerSettings &QmlDesignerPlugin::settings()
{
    NanotraceHR::Tracer tracer{"qml designer plugin settings", category()};

    return QmlDesignerBasePlugin::settings();
}

} // namespace QmlDesigner
