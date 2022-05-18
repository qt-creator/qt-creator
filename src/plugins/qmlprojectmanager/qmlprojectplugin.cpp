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

#include "qdslandingpage.h"
#include "qmlprojectplugin.h"
#include "qmlproject.h"
#include "qmlprojectconstants.h"
#include "qmlprojectrunconfiguration.h"
#include "projectfilecontenttools.h"

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/designmode.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/fileiconprovider.h>
#include <coreplugin/icore.h>
#include <coreplugin/messagebox.h>
#include <coreplugin/modemanager.h>

#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/projecttree.h>
#include <projectexplorer/runcontrol.h>
#include <projectexplorer/session.h>
#include <projectexplorer/target.h>

#include <qmljs/qmljsmodelmanagerinterface.h>

#include <qmljseditor/qmljseditor.h>
#include <qmljseditor/qmljseditorconstants.h>

#include <qmljstools/qmljstoolsconstants.h>

#include <extensionsystem/pluginmanager.h>
#include <extensionsystem/pluginspec.h>

#include <utils/fileutils.h>
#include <utils/qtcprocess.h>

#include <QAction>
#include <QDesktopServices>
#include <QMessageBox>
#include <QPointer>
#include <QPushButton>
#include <QTimer>

using namespace ProjectExplorer;

namespace QmlProjectManager {
namespace Internal {

const char alwaysOpenUiQmlMode[] = "J.QtQuick/QmlJSEditor.openUiQmlMode";
const char installQdsUrl[] = "https://www.qt.io/product/ui-design-tools";

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
    return Core::ICore::settings()->value(alwaysOpenUiQmlMode, "").toString();
}

static void setAlwaysOpenWithMode(const QString &mode)
{
    Core::ICore::settings()->setValue(alwaysOpenUiQmlMode, mode);
}

static void clearAlwaysOpenWithMode()
{
    Core::ICore::settings()->remove(alwaysOpenUiQmlMode);
}

class QmlProjectPluginPrivate
{
public:
    QmlProjectRunConfigurationFactory runConfigFactory;
    RunWorkerFactory runWorkerFactory{RunWorkerFactory::make<SimpleTargetRunner>(),
                                      {ProjectExplorer::Constants::NORMAL_RUN_MODE},
                                      {runConfigFactory.runConfigurationId()}};
    QPointer<QMessageBox> lastMessageBox;
    QdsLandingPage *landingPage = nullptr;
};

QmlProjectPlugin::~QmlProjectPlugin()
{
    if (d->lastMessageBox)
        d->lastMessageBox->deleteLater();
    if (d->landingPage)
        d->landingPage->deleteLater();
    delete d;
}

void QmlProjectPlugin::openQDS(const Utils::FilePath &fileName)
{
    const Utils::FilePath &qdsPath = QmlProjectPlugin::qdsInstallationEntry();
    bool qdsStarted = false;
    qputenv(Constants::enviromentLaunchedQDS, "true");
    //-a and -client arguments help to append project to open design studio application
    if (Utils::HostOsInfo::isMacHost())
        qdsStarted = Utils::QtcProcess::startDetached(
            {"/usr/bin/open", {"-a", qdsPath.path(), fileName.toString()}});
    else
        qdsStarted = Utils::QtcProcess::startDetached({qdsPath, {"-client", fileName.toString()}});

    if (!qdsStarted) {
        QMessageBox::warning(Core::ICore::dialogParent(),
                             fileName.fileName(),
                             QObject::tr("Failed to start Qt Design Studio."));
        if (alwaysOpenWithMode() == Core::Constants::MODE_DESIGN)
            clearAlwaysOpenWithMode();
    }
}

Utils::FilePath QmlProjectPlugin::qdsInstallationEntry()
{
    QSettings *settings = Core::ICore::settings();
    const QString qdsInstallationEntry = "QML/Designer/DesignStudioInstallation"; //set in installer

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
        QmlJS::Document::Ptr document =
                modelManager->ensuredGetDocumentForPath(editor->document()->filePath().toString());
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

    QDir dir = folder.toDir();
    if (dir.cdUp())
        return findQmlProjectUpwards(Utils::FilePath::fromString(dir.absolutePath()));
    return {};
}

static bool findAndOpenProject(const Utils::FilePath &filePath)
{

    ProjectExplorer::Project *project
            = ProjectExplorer::SessionManager::projectForFile(filePath);

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
            tr("Qt Design Studio"),
            tr("No project file (*.qmlproject) found for Qt Design "
               "Studio.\n Qt Design Studio requires a .qmlproject "
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

bool QmlProjectPlugin::initialize(const QStringList &, QString *errorMessage)
{
    Q_UNUSED(errorMessage)

    d = new QmlProjectPluginPrivate;

    if (!qmlDesignerEnabled())
        initializeQmlLandingPage();

    ProjectManager::registerProjectType<QmlProject>(QmlJSTools::Constants::QMLPROJECT_MIMETYPE);
    Core::FileIconProvider::registerIconOverlayForSuffix(":/qmlproject/images/qmlproject.png",
                                                         "qmlproject");

    if (QmlProject::isQtDesignStudio()) {
        Core::ActionContainer *menu = Core::ActionManager::actionContainer(
            ProjectExplorer::Constants::M_FILECONTEXT);
        QAction *mainfileAction = new QAction(tr("Set as main .qml file"), this);
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

        QAction *mainUifileAction = new QAction(tr("Set as main .ui.qml file"), this);
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

    return true;
}

void QmlProjectPlugin::initializeQmlLandingPage()
{
    d->landingPage = new QdsLandingPage();
    connect(d->landingPage, &QdsLandingPage::openCreator, this, &QmlProjectPlugin::openQtc);
    connect(d->landingPage, &QdsLandingPage::openDesigner, this, &QmlProjectPlugin::openQds);
    connect(d->landingPage, &QdsLandingPage::installDesigner, this, &QmlProjectPlugin::installQds);
    connect(d->landingPage, &QdsLandingPage::generateCmake, this, &QmlProjectPlugin::generateCmake);
    connect(d->landingPage, &QdsLandingPage::generateProjectFile, this, &QmlProjectPlugin::generateProjectFile);

    auto dialog = d->landingPage->dialog();

    const QStringList mimeTypes = {QmlJSTools::Constants::QMLUI_MIMETYPE};
    auto context = new Internal::DesignModeContext(dialog);
    Core::ICore::addContextObject(context);

    Core::DesignMode::registerDesignWidget(dialog, mimeTypes, context->context());

    connect(Core::ModeManager::instance(), &Core::ModeManager::currentModeChanged,
            this, &QmlProjectPlugin::editorModeChanged);
}

void QmlProjectPlugin::displayQmlLandingPage()
{
    const QString qtVersionString = ProjectFileContentTools::qtVersion(projectFilePath());
    const QString qdsVersionString = ProjectFileContentTools::qdsVersion(projectFilePath());

    d->landingPage->setQdsInstalled(qdsInstallationExists());
    d->landingPage->setProjectFileExists(projectFilePath().exists());
    d->landingPage->setCmakeResources(ProjectFileContentTools::rootCmakeFiles());
    d->landingPage->setQtVersion(qtVersionString);
    d->landingPage->setQdsVersion(qdsVersionString);
    d->landingPage->show();
}

void QmlProjectPlugin::hideQmlLandingPage()
{
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

    hideQmlLandingPage();
    Core::ModeManager::activateMode(Core::Constants::MODE_EDIT);
}

void QmlProjectPlugin::openQds(bool permanent)
{
    if (permanent)
        setAlwaysOpenWithMode(Core::Constants::MODE_DESIGN);

    hideQmlLandingPage();
    auto editor = Core::EditorManager::currentEditor();
    if (editor)
        openInQDSWithProject(editor->document()->filePath());
}

void QmlProjectPlugin::installQds()
{
    QDesktopServices::openUrl(QUrl(installQdsUrl));
    hideQmlLandingPage();
}

void QmlProjectPlugin::generateCmake()
{
    qWarning() << "TODO generate cmake";
}

void QmlProjectPlugin::generateProjectFile()
{
    qWarning() << "TODO generate .qmlproject";
}

Utils::FilePath QmlProjectPlugin::projectFilePath()
{
    auto project = ProjectExplorer::SessionManager::startupProject();
    const QmlProjectManager::QmlProject *qmlProject = qobject_cast<const QmlProjectManager::QmlProject*>(project);
    if (qmlProject)
        return qmlProject->projectFilePath();

    return {};
}

} // namespace Internal
} // namespace QmlProjectManager
