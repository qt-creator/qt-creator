// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "toolbarbackend.h"

#include "messagemodel.h"
#include "appoutputmodel.h"

#include <changestyleaction.h>
#include <crumblebar.h>
#include <designeractionmanager.h>
#include <designmodewidget.h>
#include <dynamiclicensecheck.h>
#include <qmldesignerconstants.h>
#include <qmldesignerplugin.h>
#include <qmleditormenu.h>
#include <runmanager.h>
#include <viewmanager.h>
#include <zoomaction.h>

#include <devicemanagermodel.h>
#include <devicemanagerwidget.h>

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/designmode.h>
#include <coreplugin/editormanager/documentmodel.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/modemanager.h>

#include <texteditor/textdocument.h>

#include <projectexplorer/devicesupport/devicekitaspects.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/target.h>

#include <projectexplorer/devicesupport/idevice.h>
#include <qmlprojectmanager/qmlproject.h>
#include <qtsupport/qtkitaspect.h>

#include <utils/algorithm.h>
#include <utils/qtcassert.h>

#include <QQmlEngine>

#include <memory>

namespace QmlDesigner {

static Internal::DesignModeWidget *designModeWidget()
{
    return QmlDesignerPlugin::instance()->mainWidget();
}

static DesignDocument *currentDesignDocument()
{
    QTC_ASSERT(QmlDesignerPlugin::instance(), return nullptr);

    return QmlDesignerPlugin::instance()->documentManager().currentDesignDocument();
}

static CrumbleBar *crumbleBar()
{
    return designModeWidget()->crumbleBar();
}

static Utils::FilePath getMainUiFile()
{
    auto project = ProjectExplorer::ProjectManager::startupProject();
    if (!project)
        return {};

    if (!project->activeTarget())
        return {};

    auto qmlBuildSystem = qobject_cast<QmlProjectManager::QmlBuildSystem *>(
        project->activeTarget()->buildSystem());

    if (!qmlBuildSystem)
        return {};

    return qmlBuildSystem->mainUiFilePath();
}

static void openUiFile()
{
    const Utils::FilePath mainUiFile = getMainUiFile();

    if (mainUiFile.completeSuffix() == "ui.qml" && mainUiFile.exists())
        Core::EditorManager::openEditor(mainUiFile, Utils::Id());
}

CrumbleBarModel::CrumbleBarModel(QObject *)
{
    connect(crumbleBar(), &CrumbleBar::pathChanged, this, &CrumbleBarModel::handleCrumblePathChanged);
}

int CrumbleBarModel::rowCount(const QModelIndex &) const
{
    return crumbleBar()->path().size();
}

QHash<int, QByteArray> CrumbleBarModel::roleNames() const
{
    static QHash<int, QByteArray> roleNames{{Qt::UserRole + 1, "fileName"},
                                            {Qt::UserRole + 2, "fileAddress"}};

    return roleNames;
}

QVariant CrumbleBarModel::data(const QModelIndex &index, int role) const
{
    if (index.isValid() && index.row() < rowCount()) {
        auto info = crumbleBar()->infos().at(index.row());

        if (role == Qt::UserRole + 1) {
            return info.displayName;
        } else if (role == Qt::UserRole + 2) {
            return info.fileName.displayName();
        } else {
            qWarning() << Q_FUNC_INFO << "invalid role";
        }
    } else {
        qWarning() << Q_FUNC_INFO << "invalid index";
    }

    return QVariant();
}

void CrumbleBarModel::handleCrumblePathChanged()
{
    beginResetModel();
    endResetModel();
}

void CrumbleBarModel::onCrumblePathElementClicked(int i)
{
    if (i < rowCount()) {
        auto info = crumbleBar()->infos().at(i);
        crumbleBar()->onCrumblePathElementClicked(QVariant::fromValue(info));
    }
}

WorkspaceModel::WorkspaceModel(QObject *)
{
    auto connectDockManager = [this]() -> bool {
        const auto dockManager = designModeWidget()->dockManager();
        if (!dockManager)
            return false;

        connect(dockManager, &ADS::DockManager::workspaceListChanged, this, [this] {
            beginResetModel();
            endResetModel();
        });
        beginResetModel();
        endResetModel();
        return true;
    };
    if (!connectDockManager())
        connect(designModeWidget(), &Internal::DesignModeWidget::initialized, this, connectDockManager);

    connect(ProjectExplorer::ProjectManager::instance(),
            &ProjectExplorer::ProjectManager::projectFinishedParsing,
            this,
            [this]() {
                beginResetModel();
                endResetModel();
            });
}

int WorkspaceModel::rowCount(const QModelIndex &) const
{
    if (designModeWidget() && designModeWidget()->dockManager())
        return designModeWidget()->dockManager()->workspaces().size();

    return 0;
}

QHash<int, QByteArray> WorkspaceModel::roleNames() const
{
    static QHash<int, QByteArray> roleNames{{DisplayNameRole, "displayName"},
                                            {FileNameRole, "fileName"},
                                            {Enabled, "enabled"}};

    return roleNames;
}

QVariant WorkspaceModel::data(const QModelIndex &index, int role) const
{
    if (index.isValid() && index.row() < rowCount()) {
        auto workspace = designModeWidget()->dockManager()->workspaces().at(index.row());

        if (role == DisplayNameRole) {
            return workspace.name();
        } else if (role == FileNameRole) {
            return workspace.fileName();
        } else if (role == Enabled) {
            if (QmlProjectManager::QmlProject::isMCUs())
                return workspace.isMcusEnabled();
        } else {
            qWarning() << Q_FUNC_INFO << "invalid role";
        }
    } else {
        qWarning() << Q_FUNC_INFO << "invalid index";
    }

    return QVariant();
}

RunManagerModel::RunManagerModel(QObject *)
{
    connect(&QmlDesignerPlugin::runManager(), &RunManager::targetsChanged, this, &RunManagerModel::reset);

    connect(ProjectExplorer::KitManager::instance(),
            &ProjectExplorer::KitManager::kitsChanged,
            this,
            &RunManagerModel::reset);
    connect(ProjectExplorer::KitManager::instance(),
            &ProjectExplorer::KitManager::kitsLoaded,
            this,
            &RunManagerModel::reset);

    connect(&QmlDesignerPlugin::deviceManager(),
            &DeviceShare::DeviceManager::deviceOnline,
            this,
            &RunManagerModel::reset);
    connect(&QmlDesignerPlugin::deviceManager(),
            &DeviceShare::DeviceManager::deviceOffline,
            this,
            &RunManagerModel::reset);

    connect(ProjectExplorer::ProjectManager::instance(),
            &ProjectExplorer::ProjectManager::startupProjectChanged,
            this,
            &RunManagerModel::reset);

    connect(ProjectExplorer::ProjectExplorerPlugin::instance(),
            &ProjectExplorer::ProjectExplorerPlugin::runActionsUpdated,
            this,
            &RunManagerModel::reset);
}

int RunManagerModel::rowCount(const QModelIndex &) const
{
    return QmlDesignerPlugin::runManager().targets().size();
}

QHash<int, QByteArray> RunManagerModel::roleNames() const
{
    static QHash<int, QByteArray> roleNames{{DisplayNameRole, "targetName"},
                                            {TargetNameRole, "targetId"},
                                            {Enabled, "targetEnabled"}};

    return roleNames;
}

QVariant RunManagerModel::data(const QModelIndex &index, int role) const
{
    if (index.isValid() && index.row() < rowCount()) {
        auto target = QmlDesignerPlugin::runManager().targets()[index.row()];

        if (role == DisplayNameRole)
            return std::visit([](const auto &arg) { return arg.name(); }, target);
        else if (role == TargetNameRole)
            return std::visit([](const auto &arg) { return arg.id().toSetting(); }, target);
        else if (role == Enabled)
            return std::visit([](const auto &arg) { return arg.enabled(); }, target);
        else
            qWarning() << Q_FUNC_INFO << "invalid role";
    } else {
        qWarning() << Q_FUNC_INFO << "invalid index";
    }

    return QVariant();
}

void RunManagerModel::reset()
{
    beginResetModel();
    endResetModel();
}

ActionSubscriber::ActionSubscriber(QObject *parent)
    : QObject(parent)
{
    ActionAddedInterface callback = [this](ActionInterface *interface) {
        if (interface->menuId() == m_actionId.toLatin1()) {
            m_interface = interface;
            setupNotifier();
        }
    };

    QmlDesignerPlugin::instance()->viewManager().designerActionManager().addAddActionCallback(callback);
}

void ActionSubscriber::trigger()
{
    if (m_interface)
        m_interface->action()->trigger();
}

bool ActionSubscriber::available() const
{
    if (m_interface)
        return m_interface->action()->isEnabled();
    return false;
}

bool ActionSubscriber::checked() const
{
    if (m_interface)
        return m_interface->action()->isChecked();

    return false;
}

QString ActionSubscriber::actionId() const
{
    return m_actionId;
}

void ActionSubscriber::setActionId(const QString &id)
{
    if (id == m_actionId)
        return;

    m_actionId = id;
    emit actionIdChanged();
    emit tooltipChanged();
}

QString ActionSubscriber::tooltip() const
{
    if (m_interface)
        return m_interface->action()->text();
    return {};
}

void ActionSubscriber::setupNotifier()
{
    if (!m_interface)
        return;

    connect(m_interface->action(), &QAction::enabledChanged, this, &ActionSubscriber::availableChanged);

    emit tooltipChanged();
}

ToolBarBackend::ToolBarBackend()
{
    ActionAddedInterface callback = [this](ActionInterface *interface) {
        if (interface->menuId() == "PreviewZoom")
            m_zoomAction = interface;
    };

    QmlDesignerPlugin::instance()->viewManager().designerActionManager().addAddActionCallback(callback);

    connect(designModeWidget(),
            &Internal::DesignModeWidget::navigationHistoryChanged,
            this,
            &ToolBarBackend::navigationHistoryChanged);

    connect(Core::DocumentModel::model(),
            &QAbstractItemModel::rowsInserted,
            this,
            &ToolBarBackend::updateDocumentModel);
    connect(Core::DocumentModel::model(),
            &QAbstractItemModel::rowsRemoved,
            this,
            &ToolBarBackend::updateDocumentModel);
    connect(Core::DocumentModel::model(),
            &QAbstractItemModel::dataChanged,
            this,
            &ToolBarBackend::updateDocumentModel);
    connect(Core::DocumentModel::model(),
            &QAbstractItemModel::modelReset,
            this,
            &ToolBarBackend::updateDocumentModel);

    connect(Core::EditorManager::instance(),
            &Core::EditorManager::currentEditorChanged,
            this,
            &ToolBarBackend::documentIndexChanged);

    connect(Core::EditorManager::instance(), &Core::EditorManager::currentEditorChanged, this, [this] {
        disconnect(m_documentConnection);

        if (auto textDocument = qobject_cast<TextEditor::TextDocument *>(
                Core::EditorManager::currentDocument())) {
            m_documentConnection = connect(textDocument->document(),
                                           &QTextDocument::modificationChanged,
                                           this,

                                           &ToolBarBackend::isDocumentDirtyChanged);

            emit isDocumentDirtyChanged();
        }
    });

    connect(Core::EditorManager::instance(),
            &Core::EditorManager::currentEditorChanged,
            this,
            &ToolBarBackend::currentStyleChanged);

    auto connectDockManager = [this]() -> bool {
        const auto dockManager = designModeWidget()->dockManager();
        if (!dockManager)
            return false;

        connect(dockManager,
                &ADS::DockManager::workspaceLoaded,
                this,
                &ToolBarBackend::currentWorkspaceChanged);
        connect(dockManager,
                &ADS::DockManager::workspaceListChanged,
                this,
                &ToolBarBackend::currentWorkspaceChanged);
        emit currentWorkspaceChanged();

        connect(dockManager,
                &ADS::DockManager::lockWorkspaceChanged,
                this,
                &ToolBarBackend::lockWorkspaceChanged);
        emit lockWorkspaceChanged();

        return true;
    };

    if (!connectDockManager())
        connect(designModeWidget(), &Internal::DesignModeWidget::initialized, this, connectDockManager);

    auto editorManager = Core::EditorManager::instance();

    connect(editorManager, &Core::EditorManager::documentClosed, this, [this] {
        if (isInDesignMode() && Core::DocumentModel::entryCount() == 0) {
            QTimer::singleShot(0,
                               Core::ModeManager::instance(),
                               []() { /* The mode change has to happen from event loop.
                                            Otherwise we and up in an invalid state */
                                      Core::ModeManager::activateMode(Core::Constants::MODE_WELCOME);
                               });
        }
    });

    connect(Core::ICore::instance(), &Core::ICore::coreAboutToOpen, this, [this] {
        connect(Core::DesignMode::instance(), &Core::DesignMode::enabledStateChanged, this, [this] {
            emit isDesignModeEnabledChanged();
        });
    });

    connect(Core::ModeManager::instance(), &Core::ModeManager::currentModeChanged, this, [this] {
        emit isInDesignModeChanged();
        emit isInEditModeChanged();
        emit isInSessionModeChanged();
        emit isDesignModeEnabledChanged();
    });

    connect(ProjectExplorer::ProjectManager::instance(),
            &ProjectExplorer::ProjectManager::startupProjectChanged,
            this,
            [this](ProjectExplorer::Project *project) {
                disconnect(m_kitConnection);
                emit isQt6Changed();
                emit projectOpenedChanged();
                emit stylesChanged();
                emit isMCUsChanged();
                if (project) {
                    m_kitConnection = connect(project,
                                              &ProjectExplorer::Project::activeTargetChanged,
                                              this,
                                              &ToolBarBackend::currentKitChanged);
                    emit currentKitChanged();
                }
            });

    connect(ProjectExplorer::KitManager::instance(),
            &ProjectExplorer::KitManager::kitsChanged,
            this,
            &ToolBarBackend::kitsChanged);

    // RunManager connections

    connect(&QmlDesignerPlugin::runManager(),
            &RunManager::runTargetChanged,
            this,
            &ToolBarBackend::runTargetIndexChanged);
    connect(&QmlDesignerPlugin::runManager(),
            &RunManager::runTargetTypeChanged,
            this,
            &ToolBarBackend::runTargetTypeChanged);
    connect(&QmlDesignerPlugin::runManager(),
            &RunManager::stateChanged,
            this,
            &ToolBarBackend::runManagerStateChanged);
    connect(&QmlDesignerPlugin::runManager(),
            &RunManager::progressChanged,
            this,
            &ToolBarBackend::runManagerProgressChanged);
    connect(&QmlDesignerPlugin::runManager(),
            &RunManager::errorChanged,
            this,
            &ToolBarBackend::runManagerErrorChanged);
}

void ToolBarBackend::registerDeclarativeType()
{
    qmlRegisterType<ToolBarBackend>("ToolBar", 1, 0, "ToolBarBackend");
    qmlRegisterType<ActionSubscriber>("ToolBar", 1, 0, "ActionSubscriber");
    qmlRegisterType<CrumbleBarModel>("ToolBar", 1, 0, "CrumbleBarModel");
    qmlRegisterType<WorkspaceModel>("ToolBar", 1, 0, "WorkspaceModel");
    qmlRegisterType<RunManagerModel>("ToolBar", 1, 0, "RunManagerModel");

    qmlRegisterType<MessageModel>("OutputPane", 1, 0, "MessageModel");
    qmlRegisterType<AppOutputParentModel>("OutputPane", 1, 0, "AppOutputParentModel");
    qmlRegisterType<AppOutputChildModel>("OutputPane", 1, 0, "AppOutputChildModel");

    qmlRegisterUncreatableType<RunManager>("ToolBar",
                                           1,
                                           0,
                                           "RunManager",
                                           "RunManager shouldn't be instantiated.");
    qmlRegisterUncreatableType<DeviceShare::DeviceManagerModel>(
        "ToolBar", 1, 0, "DeviceManagerModel", "DeviceManagerModel shouldn't be instantiated.");

#ifdef DVCONNECTOR_ENABLED
    qmlRegisterUncreatableType<DesignViewer::DVConnector>("ToolBar",
                                                          1,
                                                          0,
                                                          "DVConnector",
                                                          "DVConnector shouldn't be instantiated.");
#endif
}

void ToolBarBackend::triggerModeChange()
{
    QmlDesignerPlugin::emitUsageStatistics(Constants::EVENT_TOOLBAR_MODE_CHANGE);
    QTimer::singleShot(0, this, [this] { //Do not trigger mode change directly from QML
        bool qmlFileOpen = false;

        if (!projectOpened()) {
            Core::ModeManager::activateMode(Core::Constants::MODE_WELCOME);
            return;
        }

        auto document = Core::EditorManager::currentDocument();

        if (document)
            qmlFileOpen = document->filePath().fileName().endsWith(".qml");

        if (Core::ModeManager::currentModeId() == Core::Constants::MODE_DESIGN)
            Core::ModeManager::activateMode(Core::Constants::MODE_WELCOME);
        else if (qmlFileOpen)
            Core::ModeManager::activateMode(Core::Constants::MODE_DESIGN);
        else if (Core::ModeManager::currentModeId() == Core::Constants::MODE_WELCOME)
            openUiFile();
        else if (Core::ModeManager::currentModeId() == Core::Constants::MODE_EDIT)
            openUiFile();
        else
            Core::ModeManager::activateMode(Core::Constants::MODE_WELCOME);
    });
}

void ToolBarBackend::triggerProjectSettings()
{
    QmlDesignerPlugin::emitUsageStatistics(Constants::EVENT_TOOLBAR_PROJECT_SETTINGS);
    QTimer::singleShot(0, Core::ModeManager::instance(), []() { // Do not trigger mode change directly from QML
        Core::ModeManager::activateMode(ProjectExplorer::Constants::MODE_SESSION);
    });
}

void ToolBarBackend::runProject()
{
    QmlDesignerPlugin::emitUsageStatistics(Constants::EVENT_TOOLBAR_RUN_PROJECT);
    ProjectExplorer::ProjectExplorerPlugin::runStartupProject(
        ProjectExplorer::Constants::NORMAL_RUN_MODE);
}

void ToolBarBackend::goForward()
{
    QmlDesignerPlugin::emitUsageStatistics(Constants::EVENT_TOOLBAR_GO_FORWARD);
    QTC_ASSERT(designModeWidget(), return );
    designModeWidget()->toolBarOnGoForwardClicked();
}

void ToolBarBackend::goBackward()
{
    QmlDesignerPlugin::emitUsageStatistics(Constants::EVENT_TOOLBAR_GO_BACKWARD);
    QTC_ASSERT(designModeWidget(), return );
    designModeWidget()->toolBarOnGoBackClicked();
}

void ToolBarBackend::openFileByIndex(int i)
{
    QmlDesignerPlugin::emitUsageStatistics(Constants::EVENT_TOOLBAR_OPEN_FILE);
    auto fileName = Core::DocumentModel::entries().at(i)->filePath();

    Core::EditorManager::openEditor(fileName, Utils::Id(), Core::EditorManager::DoNotMakeVisible);
}

void ToolBarBackend::closeDocument(int i)
{
    QmlDesignerPlugin::emitUsageStatistics(Constants::EVENT_TOOLBAR_CLOSE_DOCUMENT);
    Core::EditorManager::closeDocument(i);
}

void ToolBarBackend::closeCurrentDocument()
{
    QmlDesignerPlugin::emitUsageStatistics(Constants::EVENT_TOOLBAR_CLOSE_CURRENT_DOCUMENT);
    Core::EditorManager::slotCloseCurrentEditorOrDocument();
}

void ToolBarBackend::shareApplicationOnline()
{
    QmlDesignerPlugin::emitUsageStatistics(Constants::EVENT_TOOLBAR_SHARE_APPLICATION);
    auto command = Core::ActionManager::command("QmlProject.ShareDesign");
    if (command)
        command->action()->trigger();
}

void ToolBarBackend::setCurrentWorkspace(const QString &workspace)
{
    QmlDesignerPlugin::emitUsageStatistics(Constants::EVENT_TOOLBAR_SET_CURRENT_WORKSPACE);
    designModeWidget()->dockManager()->openWorkspace(workspace);
}

void ToolBarBackend::setLockWorkspace(bool value)
{
    designModeWidget()->dockManager()->lockWorkspace(value);
}

void ToolBarBackend::editGlobalAnnoation()
{
    launchGlobalAnnotations();
}

void ToolBarBackend::showZoomMenu(int x, int y)
{
    return; //temporary crash fix
    QmlDesignerPlugin::emitUsageStatistics(Constants::EVENT_STATUSBAR_SHOW_ZOOM);
    ZoomAction *zoomAction = qobject_cast<ZoomAction *>(m_zoomAction->action());

    QTC_ASSERT(zoomAction, return );

    auto mainMenu = new QmlEditorMenu();

    int currentIndex = zoomAction->currentIndex();
    int i = 0;

    for (double d : zoomAction->zoomLevels()) {
        auto action = mainMenu->addAction(QString::number(d * 100) + "%");
        action->setCheckable(true);
        if (i == currentIndex)
            action->setChecked(true);
        ++i;
        connect(action, &QAction::triggered, this, [zoomAction, d] { zoomAction->setZoomFactor(d); });
    }

    mainMenu->exec(QPoint(x, y));
    mainMenu->deleteLater();
}

void ToolBarBackend::setCurrentStyle(int index)
{
    QmlDesignerPlugin::emitUsageStatistics(Constants::EVENT_STATUSBAR_SET_STYLE);
    const QList<StyleWidgetEntry> items = ChangeStyleWidgetAction::getAllStyleItems();

    QTC_ASSERT(items.size() > index, return);
    QTC_ASSERT(index >= 0, return );

    QTC_ASSERT(currentDesignDocument(), return );

    auto item = items.at(index);

    auto view = currentDesignDocument()->rewriterView();

    const QString qmlFile = view->model()->fileUrl().toLocalFile();

    ChangeStyleWidgetAction::changeCurrentStyle(item.displayName, qmlFile);

    view->resetPuppet();
}

ProjectExplorer::Kit *kitForDisplayName(const QString &displayName)
{
    const auto kits = ProjectExplorer::KitManager::kits();

    for (auto kit : kits) {
        if (kit->displayName() == displayName)
            return kit;
    }

    return {};
}

void ToolBarBackend::setCurrentKit(int index)
{
    auto project = ProjectExplorer::ProjectManager::startupProject();
    QTC_ASSERT(project, return );

    const auto kits = ToolBarBackend::kits();

    QTC_ASSERT(kits.size() > index, return );
    QTC_ASSERT(index >= 0, return );

    const auto kitDisplayName = kits.at(index);

    const auto kit = kitForDisplayName(kitDisplayName);

    QTC_ASSERT(kit, return );

    auto newTarget = project->target(kit);
    if (!newTarget)
        newTarget = project->addTargetForKit(kit);

    project->setActiveTarget(newTarget,
                             ProjectExplorer::SetActive::Cascade);

    emit currentKitChanged();
}

void ToolBarBackend::openDeviceManager()
{
    QmlDesignerPlugin::deviceManager().widget()->show();
}

void ToolBarBackend::selectRunTarget(const QString &targetName)
{
    QmlDesignerPlugin::runManager().selectRunTarget(targetName);
}

void ToolBarBackend::toggleRunning()
{
    QmlDesignerPlugin::runManager().toggleCurrentTarget();
}

void ToolBarBackend::cancelRunning()
{
    QmlDesignerPlugin::runManager().cancelCurrentTarget();
}

bool ToolBarBackend::canGoBack() const
{
    QTC_ASSERT(designModeWidget(), return false);
    return designModeWidget()->canGoBack();
}

bool ToolBarBackend::canGoForward() const
{
    QTC_ASSERT(designModeWidget(), return false);
    return designModeWidget()->canGoForward();
}

QStringList ToolBarBackend::documentModel() const
{
    return m_openDocuments;
}

void ToolBarBackend::updateDocumentModel()
{
    m_openDocuments.clear();
    for (auto &entry : Core::DocumentModel::entries())
        m_openDocuments.append(entry->displayName());

    emit openDocumentsChanged();
    emit documentIndexChanged();
}

int ToolBarBackend::documentIndex() const
{
    if (Core::EditorManager::currentDocument()) {
        std::optional index = Core::DocumentModel::indexOfDocument(
            Core::EditorManager::currentDocument());
        return index.value_or(-1);
    }

    return -1;
}

QString ToolBarBackend::currentWorkspace() const
{
    if (designModeWidget() && designModeWidget()->dockManager())
        return designModeWidget()->dockManager()->activeWorkspace()->fileName();

    return {};
}

bool ToolBarBackend::lockWorkspace() const
{
    if (designModeWidget() && designModeWidget()->dockManager())
        return designModeWidget()->dockManager()->isWorkspaceLocked();

    return false;
}

QStringList ToolBarBackend::styles() const
{
    const QList<StyleWidgetEntry> items = ChangeStyleWidgetAction::getAllStyleItems();
    QStringList list;
    for (const auto &item : items)
        list.append(item.displayName);

    return list;
}

bool ToolBarBackend::isInDesignMode() const
{
    if (!Core::ModeManager::instance())
        return false;

    return Core::ModeManager::currentModeId() == Core::Constants::MODE_DESIGN;
}

bool ToolBarBackend::isInEditMode() const
{
    if (!Core::ModeManager::instance())
        return false;

    return Core::ModeManager::currentModeId() == Core::Constants::MODE_EDIT;
}

bool ToolBarBackend::isInSessionMode() const
{
    if (!Core::ModeManager::instance())
        return false;

    return Core::ModeManager::currentModeId() == ProjectExplorer::Constants::MODE_SESSION;
}

bool ToolBarBackend::isDesignModeEnabled() const
{
    if (Core::DesignMode::instance())
        return Core::DesignMode::instance()->isEnabled() || getMainUiFile().exists();

    return false;
}

int ToolBarBackend::currentStyle() const
{
    if (currentDesignDocument()) {
        auto view = currentDesignDocument()->rewriterView();
        const QString qmlFile = view->model()->fileUrl().toLocalFile();
        return ChangeStyleWidgetAction::getCurrentStyle(qmlFile);
    } else if (Core::EditorManager::currentDocument()) {
        const QString documentPath = Core::EditorManager::currentDocument()->filePath().toFSPathString();
        return ChangeStyleWidgetAction::getCurrentStyle(documentPath);
    }

    return 0;
}

QStringList ToolBarBackend::kits() const
{
    if (!ProjectExplorer::KitManager::isLoaded())
        return {};
    auto kits = Utils::filtered(ProjectExplorer::KitManager::kits(), [](ProjectExplorer::Kit *kit) {
        const auto qtVersion = QtSupport::QtKitAspect::qtVersion(kit);
        const auto dev = ProjectExplorer::RunDeviceKitAspect::device(kit);

        return kit->isValid() && !kit->isReplacementKit() && qtVersion && qtVersion->isValid()
               && dev
            /*&& kit->isAutoDetected() */;
    });

    return Utils::transform(kits, &ProjectExplorer::Kit::displayName);
}

int ToolBarBackend::currentKit() const
{
    if (auto target = ProjectExplorer::ProjectManager::startupTarget()) {
        auto kit = target->kit();
        if (kit)
            return kits().indexOf(kit->displayName());
    }
    return 0;
}

bool ToolBarBackend::isQt6() const
{
    if (!ProjectExplorer::ProjectManager::startupTarget())
        return false;

    const QmlProjectManager::QmlBuildSystem *buildSystem = qobject_cast<QmlProjectManager::QmlBuildSystem *>(
        ProjectExplorer::ProjectManager::startupTarget()->buildSystem());
    QTC_ASSERT(buildSystem, return false);

    const bool isQt6Project = buildSystem && buildSystem->qt6Project();

    return isQt6Project;
}

bool ToolBarBackend::isMCUs() const
{
    if (!ProjectExplorer::ProjectManager::startupTarget())
        return false;

    const QmlProjectManager::QmlBuildSystem *buildSystem = qobject_cast<QmlProjectManager::QmlBuildSystem *>(
        ProjectExplorer::ProjectManager::startupTarget()->buildSystem());
    QTC_ASSERT(buildSystem, return false);

    const bool isQtForMCUsProject = buildSystem && buildSystem->qtForMCUs();

    return isQtForMCUsProject;
}

bool ToolBarBackend::projectOpened() const
{
    return ProjectExplorer::ProjectManager::instance()->startupProject();
}

bool ToolBarBackend::isDocumentDirty() const
{
    return Core::EditorManager::currentDocument()
           && Core::EditorManager::currentDocument()->isModified();
}

bool ToolBarBackend::isLiteModeEnabled() const
{
    return QmlDesignerBasePlugin::isLiteModeEnabled();
}

int ToolBarBackend::runTargetIndex() const
{
    return QmlDesignerPlugin::runManager().currentTargetIndex();
}

int ToolBarBackend::runTargetType() const
{
    return QmlDesignerPlugin::runManager().currentTargetType();
}

int ToolBarBackend::runManagerState() const
{
    return QmlDesignerPlugin::runManager().state();
}

int ToolBarBackend::runManagerProgress() const
{
    return QmlDesignerPlugin::runManager().progress();
}

QString ToolBarBackend::runManagerError() const
{
    return QmlDesignerPlugin::runManager().error();
}

#ifdef DVCONNECTOR_ENABLED
DesignViewer::DVConnector *ToolBarBackend::designViewerConnector()
{
    if (!m_designViewerConnector) {
        static constinit std::weak_ptr<DesignViewer::DVConnector> designViewerConnector;

        if (designViewerConnector.use_count() > 0) {
            m_designViewerConnector = designViewerConnector.lock();
        } else {
            m_designViewerConnector = std::make_shared<DesignViewer::DVConnector>();
            designViewerConnector = m_designViewerConnector;
        }
    }

    return m_designViewerConnector.get();
}
#endif

void ToolBarBackend::launchGlobalAnnotations()
{
    QmlDesignerPlugin::emitUsageStatistics(Constants::EVENT_TOOLBAR_EDIT_GLOBAL_ANNOTATION);

    QTC_ASSERT(currentDesignDocument(), return);
    ModelNode node = currentDesignDocument()->rewriterView()->rootModelNode();

    if (node.isValid()) {
        designModeWidget()->globalAnnotationEditor().setModelNode(node);
        designModeWidget()->globalAnnotationEditor().showWidget();
    }
}

} // namespace QmlDesigner
