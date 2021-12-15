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

#include "qmlproject.h"

#include "fileformat/qmlprojectfileformat.h"
#include "fileformat/qmlprojectitem.h"
#include "qmlprojectconstants.h"
#include "qmlprojectmanagerconstants.h"
#include "qmlprojectnodes.h"
#include "qmlprojectplugin.h"
#include "qmlprojectrunconfiguration.h"

#include <coreplugin/documentmanager.h>
#include <coreplugin/editormanager/documentmodel.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/icontext.h>
#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>

#include <projectexplorer/deploymentdata.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/target.h>

#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtkitinformation.h>
#include <qtsupport/qtsupportconstants.h>

#include <qmljs/qmljsmodelmanagerinterface.h>

#include <texteditor/textdocument.h>

#include <utils/algorithm.h>
#include <utils/infobar.h>
#include <utils/qtcprocess.h>

#include <QDebug>
#include <QLoggingCategory>
#include <QMessageBox>
#include <QRegularExpression>
#include <QTextCodec>
#include <QTimer>

using namespace Core;
using namespace ProjectExplorer;
using namespace QmlProjectManager::Internal;
using namespace Utils;

namespace {
Q_LOGGING_CATEGORY(infoLogger, "QmlProjectManager.QmlBuildSystem", QtInfoMsg)
}

namespace QmlProjectManager {

static bool isQtDesignStudio()
{
    QSettings *settings = Core::ICore::settings();
    const QString qdsStandaloneEntry = "QML/Designer/StandAloneMode"; //entry from qml settings

    return settings->value(qdsStandaloneEntry, false).toBool();
}
static int preferedQtTarget(Target *target)
{
    if (target) {
        const QmlBuildSystem *buildSystem = qobject_cast<QmlBuildSystem *>(target->buildSystem());
        if (buildSystem && buildSystem->qt6Project())
            return 6;
    }
    return 5;
}

const char openInQDSAppSetting[] = "OpenInQDSApp";

QmlProject::QmlProject(const Utils::FilePath &fileName)
    : Project(QString::fromLatin1(Constants::QMLPROJECT_MIMETYPE), fileName)
{
    setId(QmlProjectManager::Constants::QML_PROJECT_ID);
    setProjectLanguages(Context(ProjectExplorer::Constants::QMLJS_LANGUAGE_ID));
    setDisplayName(fileName.completeBaseName());

    setNeedsBuildConfigurations(false);
    setBuildSystemCreator([](Target *t) { return new QmlBuildSystem(t); });

    if (!isQtDesignStudio()) {
        if (QmlProjectPlugin::qdsInstallationExists()) {
            auto lambda = [fileName]() {
                if (Core::ICore::infoBar()->canInfoBeAdded(openInQDSAppSetting)) {
                    Utils::InfoBarEntry
                        info(openInQDSAppSetting,
                             tr("Would you like to open the project in Qt Design Studio?"),
                             Utils::InfoBarEntry::GlobalSuppression::Disabled);
                    info.addCustomButton(tr("Open in Qt Design Studio"), [&, fileName] {
                        Core::ICore::infoBar()->removeInfo(openInQDSAppSetting);
                        QmlProjectPlugin::openQDS(fileName);
                    });
                    Core::ICore::infoBar()->addInfo(info);
                }
            };

            QTimer::singleShot(0, this, lambda);
        }
    } else {
        m_openFileConnection = connect(
            this, &QmlProject::anyParsingFinished, this, [this](Target *target, bool success) {
                if (m_openFileConnection)
                    disconnect(m_openFileConnection);

                if (target && success) {
                    const Utils::FilePath &folder = projectDirectory();
                    const Utils::FilePaths &uiFiles = files([&](const ProjectExplorer::Node *node) {
                        return node->filePath().completeSuffix() == "ui.qml"
                               && node->filePath().parentDir() == folder;
                    });
                    if (!uiFiles.isEmpty()) {
                        Utils::FilePath currentFile;
                        if (auto cd = Core::EditorManager::currentDocument())
                            currentFile = cd->filePath();

                        if (currentFile.isEmpty() || !isKnownFile(currentFile))
                            Core::EditorManager::openEditor(uiFiles.first(), Utils::Id());
                    }
                }
            });
    }
}

QmlBuildSystem::QmlBuildSystem(Target *target)
    : BuildSystem(target)
{
    m_canonicalProjectDir =
            target->project()->projectFilePath().canonicalPath().normalizedPathName().parentDir();

    connect(target->project(), &Project::projectFileIsDirty,
            this, &QmlBuildSystem::refreshProjectFile);

    // refresh first - project information is used e.g. to decide the default RC's
    refresh(Everything);

// FIXME: Check. Probably bogus after the BuildSystem move.
//    // addedTarget calls updateEnabled on the runconfigurations
//    // which needs to happen after refresh
//    foreach (Target *t, targets())
//        addedTarget(t);

    connect(target->project(), &Project::activeTargetChanged,
            this, &QmlBuildSystem::onActiveTargetChanged);
    updateDeploymentData();
}

QmlBuildSystem::~QmlBuildSystem() = default;

void QmlBuildSystem::triggerParsing()
{
    refresh(Everything);
}

void QmlBuildSystem::onActiveTargetChanged(Target *)
{
    // make sure e.g. the default qml imports are adapted
    refresh(Configuration);
}

void QmlBuildSystem::onKitChanged()
{
    // make sure e.g. the default qml imports are adapted
    refresh(Configuration);
}

Utils::FilePath QmlBuildSystem::canonicalProjectDir() const
{
    return m_canonicalProjectDir;
}

void QmlBuildSystem::parseProject(RefreshOptions options)
{
    if (options & Files) {
        if (options & ProjectFile)
            m_projectItem.reset();
        if (!m_projectItem) {
            QString errorMessage;
            m_projectItem.reset(
                QmlProjectFileFormat::parseProjectFile(projectFilePath(), &errorMessage));
            if (m_projectItem) {
                connect(m_projectItem.get(),
                        &QmlProjectItem::qmlFilesChanged,
                        this,
                        &QmlBuildSystem::refreshFiles);

            } else {
                MessageManager::writeFlashing(
                    tr("Error while loading project file %1.").arg(projectFilePath().toUserOutput()));
                MessageManager::writeSilently(errorMessage);
            }
        }
        if (m_projectItem) {
            m_projectItem->setSourceDirectory(canonicalProjectDir().toString());
            if (m_projectItem->targetDirectory().isEmpty())
                m_projectItem->setTargetDirectory(canonicalProjectDir().toString());

            if (auto modelManager = QmlJS::ModelManagerInterface::instance())
                modelManager->updateSourceFiles(m_projectItem->files(), true);

            QString mainFilePath = m_projectItem->mainFile();
            if (!mainFilePath.isEmpty()) {
                mainFilePath
                        = QDir(canonicalProjectDir().toString()).absoluteFilePath(mainFilePath);
                Utils::FileReader reader;
                QString errorMessage;
                if (!reader.fetch(Utils::FilePath::fromString(mainFilePath), &errorMessage)) {
                    MessageManager::writeFlashing(tr("Warning while loading project file %1.")
                                                      .arg(projectFilePath().toUserOutput()));
                    MessageManager::writeSilently(errorMessage);
                }
            }
        }
        generateProjectTree();
    }

    if (options & Configuration) {
        // update configuration
    }
}

void QmlBuildSystem::refresh(RefreshOptions options)
{
    ParseGuard guard = guardParsingRun();
    parseProject(options);

    if (options & Files)
        generateProjectTree();

    auto modelManager = QmlJS::ModelManagerInterface::instance();
    if (!modelManager)
        return;

    QmlJS::ModelManagerInterface::ProjectInfo projectInfo =
            modelManager->defaultProjectInfoForProject(project());
    foreach (const QString &searchPath, makeAbsolute(canonicalProjectDir(), customImportPaths()))
        projectInfo.importPaths.maybeInsert(Utils::FilePath::fromString(searchPath),
                                            QmlJS::Dialect::Qml);

    modelManager->updateProjectInfo(projectInfo, project());

    guard.markAsSuccess();

    emit projectChanged();
}

QString QmlBuildSystem::mainFile() const
{
    if (m_projectItem)
        return m_projectItem->mainFile();
    return QString();
}

Utils::FilePath QmlBuildSystem::mainFilePath() const
{
    return projectDirectory().pathAppended(mainFile());
}

bool QmlBuildSystem::qtForMCUs() const
{
    if (m_projectItem)
        return m_projectItem->qtForMCUs();
    return false;
}

bool QmlBuildSystem::qt6Project() const
{
    if (m_projectItem)
        return m_projectItem->qt6Project();
    return false;
}

void QmlBuildSystem::setMainFile(const QString &mainFilePath)
{
    if (m_projectItem)
        m_projectItem->setMainFile(mainFilePath);
}

Utils::FilePath QmlBuildSystem::targetDirectory() const
{
    if (DeviceTypeKitAspect::deviceTypeId(kit())
            == ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE)
        return canonicalProjectDir();

    return m_projectItem ? Utils::FilePath::fromString(m_projectItem->targetDirectory())
                         : Utils::FilePath();
}

Utils::FilePath QmlBuildSystem::targetFile(const Utils::FilePath &sourceFile) const
{
    const QDir sourceDir(m_projectItem ? m_projectItem->sourceDirectory()
                                       : canonicalProjectDir().toString());
    const QDir targetDir(targetDirectory().toString());
    const QString relative = sourceDir.relativeFilePath(sourceFile.toString());
    return Utils::FilePath::fromString(QDir::cleanPath(targetDir.absoluteFilePath(relative)));
}

Utils::EnvironmentItems QmlBuildSystem::environment() const
{
    if (m_projectItem)
        return m_projectItem->environment();
    return {};
}

QStringList QmlBuildSystem::customImportPaths() const
{
    if (m_projectItem)
        return m_projectItem->importPaths();
    return {};
}

QStringList QmlBuildSystem::customFileSelectors() const
{
    if (m_projectItem)
        return m_projectItem->fileSelectors();
    return {};
}

bool QmlBuildSystem::multilanguageSupport() const
{
    if (m_projectItem)
        return m_projectItem->multilanguageSupport();
    return false;
}

QStringList QmlBuildSystem::supportedLanguages() const
{
    if (m_projectItem)
        return m_projectItem->supportedLanguages();
    return {};
}

void QmlBuildSystem::setSupportedLanguages(QStringList languages)
{
    if (m_projectItem)
        m_projectItem->setSupportedLanguages(languages);
}

QString QmlBuildSystem::primaryLanguage() const
{
    if (m_projectItem)
        return m_projectItem->primaryLanguage();
    return {};
}

void QmlBuildSystem::setPrimaryLanguage(QString language)
{
    if (m_projectItem)
        m_projectItem->setPrimaryLanguage(language);
}

void QmlBuildSystem::refreshProjectFile()
{
    refresh(QmlBuildSystem::ProjectFile | Files);
}

QStringList QmlBuildSystem::makeAbsolute(const Utils::FilePath &path, const QStringList &relativePaths)
{
    if (path.isEmpty())
        return relativePaths;

    const QDir baseDir(path.toString());
    return Utils::transform(relativePaths, [&baseDir](const QString &path) {
        return QDir::cleanPath(baseDir.absoluteFilePath(path));
    });
}

void QmlBuildSystem::refreshFiles(const QSet<QString> &/*added*/, const QSet<QString> &removed)
{
    if (m_blockFilesUpdate) {
        qCDebug(infoLogger) << "Auto files refresh blocked.";
        return;
    }
    refresh(Files);
    if (!removed.isEmpty()) {
        if (auto modelManager = QmlJS::ModelManagerInterface::instance())
            modelManager->removeFiles(Utils::toList(removed));
    }
    refreshTargetDirectory();
}

void QmlBuildSystem::refreshTargetDirectory()
{
    updateDeploymentData();
}

Tasks QmlProject::projectIssues(const Kit *k) const
{
    Tasks result = Project::projectIssues(k);

    const QtSupport::QtVersion *version = QtSupport::QtKitAspect::qtVersion(k);
    if (!version)
        result.append(createProjectTask(Task::TaskType::Error, tr("No Qt version set in kit.")));

    IDevice::ConstPtr dev = DeviceKitAspect::device(k);
    if (dev.isNull())
        result.append(createProjectTask(Task::TaskType::Error, tr("Kit has no device.")));

    if (version && version->qtVersion() < QtSupport::QtVersionNumber(5, 0, 0))
        result.append(createProjectTask(Task::TaskType::Error, tr("Qt version is too old.")));

    if (dev.isNull() || !version)
        return result; // No need to check deeper than this

    if (dev->type() == ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE) {
        if (version->type() == QtSupport::Constants::DESKTOPQT) {
            if (version->qmlRuntimeFilePath().isEmpty()) {
                result.append(createProjectTask(Task::TaskType::Error,
                                                tr("Qt version has no QML utility.")));
            }
        } else {
            // Non-desktop Qt on a desktop device? We don't support that.
            result.append(createProjectTask(Task::TaskType::Error,
                                            tr("Non-desktop Qt is used with a desktop device.")));
        }
    } else {
        // If not a desktop device, don't check the Qt version for qml runtime binary.
        // The device is responsible for providing it and we assume qml runtime can be found
        // in $PATH if it's not explicitly given.
    }

    return result;
}

Project::RestoreResult QmlProject::fromMap(const QVariantMap &map, QString *errorMessage)
{
    RestoreResult result = Project::fromMap(map, errorMessage);
    if (result != RestoreResult::Ok)
        return result;

    if (!activeTarget()) {
        // find a kit that matches prerequisites (prefer default one)
        const QList<Kit *> kits = Utils::filtered(KitManager::kits(), [this](const Kit *k) {
            return !containsType(projectIssues(k), Task::TaskType::Error)
                   && DeviceTypeKitAspect::deviceTypeId(k)
                          == ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE;
        });

        if (!kits.isEmpty()) {
            if (kits.contains(KitManager::defaultKit()))
                addTargetForDefaultKit();
            else
                addTargetForKit(kits.first());
        }

        if (isQtDesignStudio()) {
            auto setKitWithVersion = [&](int qtMajorVersion) {
                const QList<Kit *> qtVersionkits
                    = Utils::filtered(kits, [qtMajorVersion](const Kit *k) {
                          QtSupport::QtVersion *version = QtSupport::QtKitAspect::qtVersion(k);
                          return (version && version->qtVersion().majorVersion == qtMajorVersion);
                      });
                if (!qtVersionkits.isEmpty()) {
                    if (qtVersionkits.contains(KitManager::defaultKit()))
                        addTargetForDefaultKit();
                    else
                        addTargetForKit(qtVersionkits.first());
                }
            };

            int preferedVersion = preferedQtTarget(activeTarget());

            if (activeTarget())
                removeTarget(activeTarget());

            setKitWithVersion(preferedVersion);
        }
    }

    return RestoreResult::Ok;
}

ProjectExplorer::DeploymentKnowledge QmlProject::deploymentKnowledge() const
{
    return DeploymentKnowledge::Perfect;
}

void QmlBuildSystem::generateProjectTree()
{
    if (!m_projectItem)
        return;

    auto newRoot = std::make_unique<QmlProjectNode>(project());

    for (const QString &f : m_projectItem->files()) {
        const Utils::FilePath fileName = Utils::FilePath::fromString(f);
        const FileType fileType = (fileName == projectFilePath())
                ? FileType::Project : FileNode::fileTypeForFileName(fileName);
        newRoot->addNestedNode(std::make_unique<FileNode>(fileName, fileType));
    }
    newRoot->addNestedNode(std::make_unique<FileNode>(projectFilePath(), FileType::Project));

    setRootProjectNode(std::move(newRoot));
    refreshTargetDirectory();
}

void QmlBuildSystem::updateDeploymentData()
{
    if (!m_projectItem)
        return;

    if (DeviceTypeKitAspect::deviceTypeId(kit())
            == ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE) {
        return;
    }

    ProjectExplorer::DeploymentData deploymentData;
    for (const QString &file : m_projectItem->files()) {
        deploymentData.addFile(
                    FilePath::fromString(file),
                    targetFile(Utils::FilePath::fromString(file)).parentDir().toString());
    }

    setDeploymentData(deploymentData);
}

QVariant QmlBuildSystem::additionalData(Id id) const
{
    if (id == Constants::customFileSelectorsData)
        return customFileSelectors();
    if (id == Constants::supportedLanguagesData)
        return supportedLanguages();
    if (id == Constants::primaryLanguageData)
        return primaryLanguage();
    if (id == Constants::customForceFreeTypeData)
        return forceFreeType();
    if (id == Constants::customQtForMCUs)
        return qtForMCUs();
    if (id == Constants::customQt6Project)
        return qt6Project();
    if (id == Constants::mainFilePath)
        return mainFilePath().toString();
    if (id == Constants::customImportPaths)
        return customImportPaths();
    if (id == Constants::canonicalProjectDir)
        return canonicalProjectDir().toString();
    return {};
}

bool QmlBuildSystem::supportsAction(Node *context, ProjectAction action, const Node *node) const
{
    if (dynamic_cast<QmlProjectNode *>(context)) {
        if (action == AddNewFile || action == EraseFile)
            return true;
        QTC_ASSERT(node, return false);

        if (action == Rename && node->asFileNode()) {
            const FileNode *fileNode = node->asFileNode();
            QTC_ASSERT(fileNode, return false);
            return fileNode->fileType() != FileType::Project;
        }

        return false;
    }

    return BuildSystem::supportsAction(context, action, node);
}

QmlProject *QmlBuildSystem::qmlProject() const
{
    return static_cast<QmlProject *>(BuildSystem::project());
}

bool QmlBuildSystem::forceFreeType() const
{
    if (m_projectItem)
        return m_projectItem->forceFreeType();
    return false;
}

bool QmlBuildSystem::widgetApp() const
{
    if (m_projectItem)
        return m_projectItem->widgetApp();
    return false;
}

QStringList QmlBuildSystem::importPaths() const
{
    if (m_projectItem)
        return m_projectItem->importPaths();
    return {};
}

QStringList QmlBuildSystem::files() const
{
    if (m_projectItem)
        return m_projectItem->files();
    return {};
}

bool QmlBuildSystem::addFiles(Node *context, const FilePaths &filePaths, FilePaths *)
{
    if (!dynamic_cast<QmlProjectNode *>(context))
        return false;

    FilePaths toAdd;
    for (const FilePath &filePath : filePaths) {
        if (!m_projectItem->matchesFile(filePath.toString()))
            toAdd << filePaths;
    }
    return toAdd.isEmpty();
}

bool QmlBuildSystem::deleteFiles(Node *context, const FilePaths &filePaths)
{
    if (dynamic_cast<QmlProjectNode *>(context))
        return true;

    return BuildSystem::deleteFiles(context, filePaths);
}

bool QmlBuildSystem::renameFile(Node * context, const FilePath &oldFilePath, const FilePath &newFilePath)
{
    if (dynamic_cast<QmlProjectNode *>(context)) {
        if (oldFilePath.endsWith(mainFile())) {
            setMainFile(newFilePath.toString());

            // make sure to change it also in the qmlproject file
            const Utils::FilePath qmlProjectFilePath = project()->projectFilePath();
            Core::FileChangeBlocker fileChangeBlocker(qmlProjectFilePath);
            const QList<Core::IEditor *> editors = Core::DocumentModel::editorsForFilePath(qmlProjectFilePath);
            TextEditor::TextDocument *document = nullptr;
            if (!editors.isEmpty()) {
                document = qobject_cast<TextEditor::TextDocument*>(editors.first()->document());
                if (document && document->isModified())
                    if (!Core::DocumentManager::saveDocument(document))
                        return false;
            }

            QString fileContent;
            QString error;
            Utils::TextFileFormat textFileFormat;
            const QTextCodec *codec = QTextCodec::codecForName("UTF-8"); // qml files are defined to be utf-8
            if (Utils::TextFileFormat::readFile(qmlProjectFilePath, codec, &fileContent, &textFileFormat, &error)
                    != Utils::TextFileFormat::ReadSuccess) {
                qWarning() << "Failed to read file" << qmlProjectFilePath << ":" << error;
            }

            // find the mainFile and do the file name with brackets in a capture group and mask the . with \.
            QString originalFileName = oldFilePath.fileName();
            originalFileName.replace(".", "\\.");
            const QRegularExpression expression(QString("mainFile:\\s*\"(%1)\"").arg(originalFileName));
            const QRegularExpressionMatch match = expression.match(fileContent);

            fileContent.replace(match.capturedStart(1), match.capturedLength(1), newFilePath.fileName());

            if (!textFileFormat.writeFile(qmlProjectFilePath, fileContent, &error))
                qWarning() << "Failed to write file" << qmlProjectFilePath << ":" << error;

            refresh(Everything);
        }

        return true;
    }

    return BuildSystem::renameFile(context, oldFilePath, newFilePath);
}

} // namespace QmlProjectManager

