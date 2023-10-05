// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qdslandingpage.h"
#include "qmlprojectplugin.h"
#include "qmlproject.h"
#include "qmlprojectconstants.h"
#include "qmlprojectmanagertr.h"
#include "qmlprojectrunconfiguration.h"
#include "projectfilecontenttools.h"
#include "cmakegen/cmakeprojectconverter.h"
#include "cmakegen/generatecmakelists.h"

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/designmode.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/messagebox.h>
#include <coreplugin/modemanager.h>

#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/projecttree.h>
#include <projectexplorer/runcontrol.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/target.h>

#include <qmljs/qmljsmodelmanagerinterface.h>

#include <qmljseditor/qmljseditor.h>
#include <qmljseditor/qmljseditorconstants.h>

#include <qmljstools/qmljstoolsconstants.h>

#include <extensionsystem/pluginmanager.h>
#include <extensionsystem/pluginspec.h>

#include <utils/fileutils.h>
#include <utils/fsengine/fileiconprovider.h>
#include <utils/process.h>
#include <utils/qtcsettings.h>

#include <QAction>
#include <QDesktopServices>
#include <QMessageBox>
#include <QPointer>
#include <QPushButton>
#include <QTimer>

using namespace ProjectExplorer;
using namespace Utils;

namespace QmlProjectManager::Internal {

static bool isQmlDesigner(const ExtensionSystem::PluginSpec *spec)
{
    if (!spec)
        return false;

    return spec->name().contains("QmlDesigner");
}

static bool qmlDesignerEnabled()
{
    const auto plugins = ExtensionSystem::PluginManager::plugins();
    const auto it = std::find_if(plugins.begin(), plugins.end(), &isQmlDesigner);
    return it != plugins.end() && (*it)->plugin();
}

static QString alwaysOpenWithMode()
{
    return Core::ICore::settings()
        ->value(QmlProjectManager::Constants::ALWAYS_OPEN_UI_MODE, "")
        .toString();
}

static void setAlwaysOpenWithMode(const QString &mode)
{
    Core::ICore::settings()->setValue(QmlProjectManager::Constants::ALWAYS_OPEN_UI_MODE, mode);
}

static void clearAlwaysOpenWithMode()
{
    Core::ICore::settings()->remove(QmlProjectManager::Constants::ALWAYS_OPEN_UI_MODE);
}

class QmlProjectPluginPrivate
{
public:
    QmlProjectRunConfigurationFactory runConfigFactory;
    SimpleTargetRunnerFactory runWorkerFactory{{runConfigFactory.runConfigurationId()}};
    QPointer<QMessageBox> lastMessageBox;
    QdsLandingPage *landingPage = nullptr;
    QdsLandingPageWidget *landingPageWidget = nullptr;
};

QmlProjectPlugin::~QmlProjectPlugin()
{
    QTC_ASSERT(d, return);

    if (d->lastMessageBox)
        d->lastMessageBox->deleteLater();
    if (d->landingPage)
        d->landingPage->deleteLater();
    if (d->landingPageWidget)
        d->landingPageWidget->deleteLater();
    delete d;
}

void QmlProjectPlugin::openQDS(const Utils::FilePath &fileName)
{
    const Utils::FilePath &qdsPath = QmlProjectPlugin::qdsInstallationEntry();
    bool qdsStarted = false;
    qputenv(Constants::enviromentLaunchedQDS, "true");
    //-a and -client arguments help to append project to open design studio application
    if (Utils::HostOsInfo::isMacHost())
        qdsStarted = Utils::Process::startDetached(
            {"/usr/bin/open", {"-a", qdsPath.path(), fileName.toString()}});
    else
        qdsStarted = Utils::Process::startDetached({qdsPath, {"-client", fileName.toString()}});

    if (!qdsStarted) {
        QMessageBox::warning(Core::ICore::dialogParent(),
                             fileName.fileName(),
                             Tr::tr("Failed to start Qt Design Studio."));
        if (alwaysOpenWithMode() == Core::Constants::MODE_DESIGN)
            clearAlwaysOpenWithMode();
    }
}

Utils::FilePath QmlProjectPlugin::qdsInstallationEntry()
{
    QtcSettings *settings = Core::ICore::settings();
    const Key qdsInstallationEntry = "QML/Designer/DesignStudioInstallation"; //set in installer

    return Utils::FilePath::fromUserInput(settings->value(qdsInstallationEntry).toString());
}

bool QmlProjectPlugin::qdsInstallationExists()
{
    return qdsInstallationEntry().exists();
}

bool QmlProjectPlugin::checkIfEditorIsuiQml(Core::IEditor *editor)
{
    if (editor
        && (editor->document()->id() == QmlJSEditor::Constants::C_QMLJSEDITOR_ID
            || editor->document()->id() == QmlJSEditor::Constants::C_QTQUICKDESIGNEREDITOR_ID)) {
        QmlJS::ModelManagerInterface *modelManager = QmlJS::ModelManagerInterface::instance();
        QmlJS::Document::Ptr document = modelManager->ensuredGetDocumentForPath(
            editor->document()->filePath());
        if (!document.isNull())
            return document->language() == QmlJS::Dialect::QmlQtQuick2Ui;
    }
    return false;
}

const Utils::FilePath findQmlProject(const Utils::FilePath &folder)
{
    const Utils::FilePaths files = folder.dirEntries({QStringList("*.qmlproject"), QDir::Files});
    if (files.isEmpty())
        return {};

    return files.constFirst();
}

const Utils::FilePath findQmlProjectUpwards(const Utils::FilePath &folder)
{
    auto ret = findQmlProject(folder);
    if (ret.exists())
        return ret;

    if (folder.parentDir().isDir())
        return findQmlProjectUpwards(folder.parentDir());

    return {};
}

static bool findAndOpenProject(const Utils::FilePath &filePath)
{
    ProjectExplorer::Project *project
            = ProjectExplorer::ProjectManager::projectForFile(filePath);

    if (project) {
        if (project->projectFilePath().suffix() == "qmlproject") {
            QmlProjectPlugin::openQDS(project->projectFilePath());
            return true;
        } else {
            auto projectFolder = project->rootProjectDirectory();
            auto qmlProjectFile = findQmlProject(projectFolder);
            if (qmlProjectFile.exists()) {
                QmlProjectPlugin::openQDS(qmlProjectFile);
                return true;
            }
        }
    }

    auto qmlProjectFile = findQmlProjectUpwards(filePath);
    if (qmlProjectFile.exists()) {
        QmlProjectPlugin::openQDS(qmlProjectFile);
        return true;
    }
    return false;
}

void QmlProjectPlugin::openInQDSWithProject(const Utils::FilePath &filePath)
{
    if (findAndOpenProject(filePath)) {
        openQDS(filePath);
        //The first one might be ignored when QDS is starting up
        QTimer::singleShot(4000, [filePath] { openQDS(filePath); });
    } else {
        Core::AsynchronousMessageBox::warning(
            Tr::tr("Qt Design Studio"),
            Tr::tr("No project file (*.qmlproject) found for Qt Design "
               "Studio.\nQt Design Studio requires a .qmlproject "
               "based project to open the .ui.qml file."));
    }
}

static QmlBuildSystem *qmlBuildSystemforFileNode(const FileNode *fileNode)
{
    if (!fileNode)
        return nullptr;

    if (QmlProject *qmlProject = qobject_cast<QmlProject*>(fileNode->getProject())) {
        auto target = qmlProject->activeTarget();
        if (!target)
            return nullptr;

        return qobject_cast<QmlProjectManager::QmlBuildSystem *>(target->buildSystem());

    }

    return nullptr;
}

void QmlProjectPlugin::initialize()
{
    d = new QmlProjectPluginPrivate;

    if (!qmlDesignerEnabled()) {
        d->landingPage = new QdsLandingPage();
        qmlRegisterSingletonInstance<QdsLandingPage>("LandingPageApi",
                                                     1,
                                                     0,
                                                     "LandingPageApi",
                                                     d->landingPage);

        d->landingPageWidget = new QdsLandingPageWidget();

        const QStringList mimeTypes = {QmlJSTools::Constants::QMLUI_MIMETYPE};
        auto context = new Internal::DesignModeContext(d->landingPageWidget);
        Core::ICore::addContextObject(context);

        Core::DesignMode::registerDesignWidget(d->landingPageWidget, mimeTypes, context->context());

        connect(Core::ModeManager::instance(), &Core::ModeManager::currentModeChanged,
                this, &QmlProjectPlugin::editorModeChanged);
    }

    ProjectManager::registerProjectType<QmlProject>(QmlJSTools::Constants::QMLPROJECT_MIMETYPE);
    Utils::FileIconProvider::registerIconOverlayForSuffix(":/qmlproject/images/qmlproject.png",
                                                         "qmlproject");

    if (Core::ICore::isQtDesignStudio()) {
        Core::ActionContainer *menu = Core::ActionManager::actionContainer(
            ProjectExplorer::Constants::M_FILECONTEXT);
        QAction *mainfileAction = new QAction(Tr::tr("Set as Main .qml File"), this);
        mainfileAction->setEnabled(false);

        connect(mainfileAction, &QAction::triggered, this, []() {
            const Node *currentNode = ProjectTree::currentNode();
            if (!currentNode || !currentNode->asFileNode()
                || currentNode->asFileNode()->fileType() != FileType::QML)
                return;

            const Utils::FilePath file = currentNode->filePath();

            QmlBuildSystem *buildSystem = qmlBuildSystemforFileNode(currentNode->asFileNode());
            if (buildSystem)
                buildSystem->setMainFileInProjectFile(file);
        });

        menu->addAction(Core::ActionManager::registerAction(
                            mainfileAction,
                            "QmlProject.setMainFile",
                            Core::Context(ProjectExplorer::Constants::C_PROJECT_TREE)),
                        ProjectExplorer::Constants::G_FILE_OTHER);
        mainfileAction->setVisible(false);
        connect(ProjectTree::instance(),
                &ProjectTree::currentNodeChanged,
                mainfileAction,
                [mainfileAction](Node *node) {
                    const FileNode *fileNode = node ? node->asFileNode() : nullptr;

                    const bool isVisible = fileNode && fileNode->fileType() == FileType::QML
                                           && fileNode->filePath().completeSuffix() == "qml";

                    mainfileAction->setVisible(isVisible);

                    if (!isVisible)
                        return;

                    QmlBuildSystem *buildSystem = qmlBuildSystemforFileNode(fileNode);

                    if (buildSystem)
                        mainfileAction->setEnabled(buildSystem->mainFilePath()
                                                   != fileNode->filePath());
                });

        QAction *mainUifileAction = new QAction(Tr::tr("Set as Main .ui.qml File"), this);
        mainUifileAction->setEnabled(false);

        connect(mainUifileAction, &QAction::triggered, this, []() {
            const Node *currentNode = ProjectTree::currentNode();
            if (!currentNode || !currentNode->asFileNode()
                || currentNode->asFileNode()->fileType() != FileType::QML)
                return;

            const Utils::FilePath file = currentNode->filePath();

            QmlBuildSystem *buildSystem = qmlBuildSystemforFileNode(currentNode->asFileNode());
            if (buildSystem)
                buildSystem->setMainUiFileInProjectFile(file);
        });

        menu->addAction(Core::ActionManager::registerAction(
                            mainUifileAction,
                            "QmlProject.setMainUIFile",
                            Core::Context(ProjectExplorer::Constants::C_PROJECT_TREE)),
                        ProjectExplorer::Constants::G_FILE_OTHER);
        mainUifileAction->setVisible(false);
        connect(ProjectTree::instance(),
                &ProjectTree::currentNodeChanged,
                mainUifileAction,
                [mainUifileAction](Node *node) {
                    const FileNode *fileNode = node ? node->asFileNode() : nullptr;
                    const bool isVisible = fileNode && fileNode->fileType() == FileType::QML
                                           && fileNode->filePath().completeSuffix() == "ui.qml";

                    mainUifileAction->setVisible(isVisible);

                    if (!isVisible)
                        return;

                    QmlBuildSystem *buildSystem = qmlBuildSystemforFileNode(fileNode);
                    if (buildSystem)
                        mainUifileAction->setEnabled(buildSystem->mainUiFilePath()
                                                     != fileNode->filePath());
                });
    }

    GenerateCmake::generateMenuEntry(this);
    if (Core::ICore::isQtDesignStudio())
        GenerateCmake::CmakeProjectConverter::generateMenuEntry(this);
}

void QmlProjectPlugin::displayQmlLandingPage()
{
    if (!d->landingPage)
        return;

    d->landingPage->setWidget(d->landingPageWidget->widget());

    updateQmlLandingPageProjectInfo(projectFilePath());
    d->landingPage->setQdsInstalled(qdsInstallationExists());
    d->landingPage->setCmakeResources(ProjectFileContentTools::rootCmakeFiles());
    d->landingPage->show();
}

void QmlProjectPlugin::hideQmlLandingPage()
{
    if (d->landingPage)
        d->landingPage->hide();
}

static bool isDesignerMode(Utils::Id mode)
{
    return mode == Core::Constants::MODE_DESIGN;
}

void QmlProjectPlugin::editorModeChanged(Utils::Id newMode, Utils::Id oldMode)
{
    Core::IEditor *currentEditor = Core::EditorManager::currentEditor();
    if (checkIfEditorIsuiQml(currentEditor)) {
        if (isDesignerMode(newMode)) {
            if (alwaysOpenWithMode() == Core::Constants::MODE_DESIGN)
                openQds();
            else if (alwaysOpenWithMode() == Core::Constants::MODE_EDIT)
                openQtc();
            else
                displayQmlLandingPage();
        } else if (isDesignerMode(oldMode)) {
            hideQmlLandingPage();
        }
    }
}

void QmlProjectPlugin::openQtc(bool permanent)
{
    if (permanent)
        setAlwaysOpenWithMode(Core::Constants::MODE_EDIT);

    if (d->landingPage)
        hideQmlLandingPage();

    Core::ModeManager::activateMode(Core::Constants::MODE_EDIT);
}

void QmlProjectPlugin::openQds(bool permanent)
{
    if (permanent)
        setAlwaysOpenWithMode(Core::Constants::MODE_DESIGN);

    if (d->landingPage)
        hideQmlLandingPage();

    auto editor = Core::EditorManager::currentEditor();
    if (editor)
        openInQDSWithProject(editor->document()->filePath());
}

void QmlProjectPlugin::updateQmlLandingPageProjectInfo(const Utils::FilePath &projectFile)
{
    if (!d->landingPage)
        return;

    const QString qtVersionString = ProjectFileContentTools::qtVersion(projectFile);
    const QString qdsVersionString = ProjectFileContentTools::qdsVersion(projectFile);
    d->landingPage->setProjectFileExists(projectFile.exists());
    d->landingPage->setQtVersion(qtVersionString);
    d->landingPage->setQdsVersion(qdsVersionString);
}

Utils::FilePath QmlProjectPlugin::projectFilePath()
{
    auto project = ProjectExplorer::ProjectManager::startupProject();
    const QmlProjectManager::QmlProject *qmlProject = qobject_cast<const QmlProjectManager::QmlProject*>(project);
    if (qmlProject) {
        return qmlProject->projectFilePath();
    } else if (project) {
        auto projectFolder = project->rootProjectDirectory();
        auto qmlProjectFile = findQmlProject(projectFolder);
        if (qmlProjectFile.exists())
            return qmlProjectFile;
    }

    return {};
}

} // QmlProjectManager::Internal
