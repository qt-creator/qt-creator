// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "toolbarbackend.h"

#include <crumblebar.h>
#include <designeractionmanager.h>
#include <designmodewidget.h>
#include <viewmanager.h>
#include <qmldesignerplugin.h>

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/designmode.h>
#include <coreplugin/editormanager/documentmodel.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/modemanager.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/session.h>
#include <projectexplorer/target.h>

#include <qmlprojectmanager/qmlproject.h>

#include <utils/algorithm.h>
#include <utils/qtcassert.h>

#include <QQmlEngine>

namespace QmlDesigner {

static Internal::DesignModeWidget *designModeWidget()
{
    return QmlDesignerPlugin::instance()->mainWidget();
}

static DesignDocument *currentDesignDocument()
{
    return QmlDesignerPlugin::instance()->documentManager().currentDesignDocument();
}

static CrumbleBar *crumbleBar()
{
    return designModeWidget()->crumbleBar();
}

static Utils::FilePath getMainUiFile()
{
    auto project = ProjectExplorer::SessionManager::startupProject();
    if (!project)
        return {};

    if (!project->activeTarget())
        return {};

    auto qmlBuildSystem = qobject_cast<QmlProjectManager::QmlBuildSystem *>(
        project->activeTarget()->buildSystem());
    return qmlBuildSystem->mainUiFilePath();
}

static void openUiFile()
{
    const Utils::FilePath mainUiFile = getMainUiFile();

    if (mainUiFile.completeSuffix() == "ui.qml" && mainUiFile.exists())
        Core::EditorManager::openEditor(mainUiFile, Utils::Id());
}

void ToolBarBackend::triggerModeChange()
{
    QTimer::singleShot(0, []() { //Do not trigger mode change directly from QML
        bool qmlFileOpen = false;

        auto document = Core::EditorManager::currentDocument();

        if (document)
            qmlFileOpen = document->filePath().fileName().endsWith(".qml");

        if (Core::ModeManager::currentModeId() == Core::Constants::MODE_DESIGN)
            Core::ModeManager::activateMode(Core::Constants::MODE_WELCOME);
        else if (qmlFileOpen)
            Core::ModeManager::activateMode(Core::Constants::MODE_DESIGN);
        else if (Core::ModeManager::currentModeId() == Core::Constants::MODE_WELCOME)
            openUiFile();
        else
            Core::ModeManager::activateMode(Core::Constants::MODE_WELCOME);
    });
}

void ToolBarBackend::runProject()
{
    ProjectExplorer::ProjectExplorerPlugin::runStartupProject(
        ProjectExplorer::Constants::NORMAL_RUN_MODE);
}

void ToolBarBackend::goForward()
{
    QTC_ASSERT(designModeWidget(), return );
    designModeWidget()->toolBarOnGoForwardClicked();
}

void ToolBarBackend::goBackward()
{
    QTC_ASSERT(designModeWidget(), return );
    designModeWidget()->toolBarOnGoBackClicked();
}

void ToolBarBackend::openFileByIndex(int i)
{
    auto fileName = Core::DocumentModel::entries().at(i)->fileName();

    Core::EditorManager::openEditor(fileName, Utils::Id(), Core::EditorManager::DoNotMakeVisible);
}

void ToolBarBackend::closeCurrentDocument()
{
    Core::EditorManager::slotCloseCurrentEditorOrDocument();
}

void ToolBarBackend::shareApplicationOnline()
{
    auto command = Core::ActionManager::command("QmlProject.ShareDesign");
    if (command)
        command->action()->trigger();
}

void ToolBarBackend::setCurrentWorkspace(const QString &workspace)
{
    designModeWidget()->dockManager()->openWorkspace(workspace);
}

void ToolBarBackend::editGlobalAnnoation()
{
    ModelNode node = currentDesignDocument()->rewriterView()->rootModelNode();

    if (node.isValid()) {
        designModeWidget()->globalAnnotationEditor().setModelNode(node);
        designModeWidget()->globalAnnotationEditor().showWidget();
    }
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

ToolBarBackend::ToolBarBackend(QObject *parent)
    : QObject(parent)
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

    connect(designModeWidget(), &Internal::DesignModeWidget::initialized, this, [this]() {
        const auto dockManager = designModeWidget()->dockManager();

        connect(dockManager,
                &ADS::DockManager::workspaceListChanged,
                this,
                &ToolBarBackend::setupWorkspaces);
        connect(dockManager, &ADS::DockManager::workspaceLoaded, this, [this](const QString &) {
            emit currentWorkspaceChanged();
        });

        setupWorkspaces();
    });

    auto editorManager = Core::EditorManager::instance();

    connect(editorManager, &Core::EditorManager::documentClosed, this, [this]() {
        if (isInDesignMode() && Core::DocumentModel::entryCount() == 0) {
            QTimer::singleShot(0, []() { /* The mode change has to happen from event loop.
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

    connect(Core::ModeManager::instance(), &Core::ModeManager::currentModeChanged, this, [this]() {
        emit isInDesignModeChanged();
    });
}

void ToolBarBackend::registerDeclarativeType()
{
    qmlRegisterType<ToolBarBackend>("ToolBar", 1, 0, "ToolBarBackend");
    qmlRegisterType<ActionSubscriber>("ToolBar", 1, 0, "ActionSubscriber");
    qmlRegisterType<CrumbleBarModel>("ToolBar", 1, 0, "CrumbleBarModel");
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
}

int ToolBarBackend::documentIndex() const
{
    if (Core::EditorManager::currentDocument())
        return Core::DocumentModel::indexOfDocument(Core::EditorManager::currentDocument()).value();

    return -1;
}

QString ToolBarBackend::currentWorkspace() const
{
    if (designModeWidget() && designModeWidget()->dockManager())
        return designModeWidget()->dockManager()->activeWorkspace();
    return {};
}

QStringList ToolBarBackend::workspaces() const
{
    return m_workspaces;
}

bool ToolBarBackend::isInDesignMode() const
{
    if (!Core::ModeManager::instance())
        return false;

    return Core::ModeManager::currentModeId() == Core::Constants::MODE_DESIGN;
}

bool ToolBarBackend::isDesignModeEnabled() const
{
    if (Core::DesignMode::instance())
        return Core::DesignMode::instance()->isEnabled() || getMainUiFile().exists();

    return false;
}

void ToolBarBackend::setupWorkspaces()
{
    m_workspaces.clear();
    m_workspaces = designModeWidget()->dockManager()->workspaces();
    Utils::sort(m_workspaces);
    emit workspacesChanged();
    emit currentWorkspaceChanged();
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

CrumbleBarModel::CrumbleBarModel(QObject *)
{
    connect(crumbleBar(), &CrumbleBar::pathChanged, this, &CrumbleBarModel::handleCrumblePathChanged);
}

int CrumbleBarModel::rowCount(const QModelIndex &) const
{
    return crumbleBar()->path().count();
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

} // namespace QmlDesigner
