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
#include "qmlprojectrunconfiguration.h"
#include "qmlprojectconstants.h"
#include "qmlprojectmanagerconstants.h"
#include "qmlprojectnodes.h"

#include <coreplugin/documentmanager.h>
#include <coreplugin/editormanager/documentmodel.h>
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

#include <QDebug>
#include <QRegularExpression>
#include <QTextCodec>
#include <QLoggingCategory>

using namespace Core;
using namespace ProjectExplorer;
using namespace QmlProjectManager::Internal;
using namespace Utils;

namespace {
Q_LOGGING_CATEGORY(infoLogger, "QmlProjectManager.QmlBuildSystem", QtInfoMsg)
}

namespace QmlProjectManager {

QmlProject::QmlProject(const Utils::FilePath &fileName)
    : Project(QString::fromLatin1(Constants::QMLPROJECT_MIMETYPE), fileName)
{
    setId(QmlProjectManager::Constants::QML_PROJECT_ID);
    setProjectLanguages(Context(ProjectExplorer::Constants::QMLJS_LANGUAGE_ID));
    setDisplayName(fileName.toFileInfo().completeBaseName());

    setNeedsBuildConfigurations(false);
    setBuildSystemCreator([](Target *t) { return new QmlBuildSystem(t); });
}

QmlBuildSystem::QmlBuildSystem(Target *target)
    : BuildSystem(target)
{
    const QString normalized
            = Utils::FileUtils::normalizePathName(target->project()
                      ->projectFilePath().toFileInfo().canonicalFilePath());
    m_canonicalProjectDir = Utils::FilePath::fromString(normalized).parentDir();

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

QmlBuildSystem::~QmlBuildSystem()
{
    delete m_projectItem.data();
}

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
            delete m_projectItem.data();
        if (!m_projectItem) {
              QString errorMessage;
              m_projectItem = QmlProjectFileFormat::parseProjectFile(projectFilePath(), &errorMessage);
              if (m_projectItem) {
                  connect(m_projectItem.data(), &QmlProjectItem::qmlFilesChanged,
                          this, &QmlBuildSystem::refreshFiles);

              } else {
                  MessageManager::write(tr("Error while loading project file %1.")
                                        .arg(projectFilePath().toUserOutput()),
                                        MessageManager::NoModeSwitch);
                  MessageManager::write(errorMessage);
              }
        }
        if (m_projectItem) {
            m_projectItem.data()->setSourceDirectory(canonicalProjectDir().toString());
            if (m_projectItem->targetDirectory().isEmpty())
                m_projectItem->setTargetDirectory(canonicalProjectDir().toString());

            if (auto modelManager = QmlJS::ModelManagerInterface::instance())
                modelManager->updateSourceFiles(m_projectItem.data()->files(), true);

            QString mainFilePath = m_projectItem.data()->mainFile();
            if (!mainFilePath.isEmpty()) {
                mainFilePath
                        = QDir(canonicalProjectDir().toString()).absoluteFilePath(mainFilePath);
                Utils::FileReader reader;
                QString errorMessage;
                if (!reader.fetch(mainFilePath, &errorMessage)) {
                    MessageManager::write(tr("Warning while loading project file %1.")
                                          .arg(projectFilePath().toUserOutput()));
                    MessageManager::write(errorMessage);
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
}

QString QmlBuildSystem::mainFile() const
{
    if (m_projectItem)
        return m_projectItem.data()->mainFile();
    return QString();
}

bool QmlBuildSystem::qtForMCUs() const
{
    if (m_projectItem)
        return m_projectItem.data()->qtForMCUs();
    return false;
}

void QmlBuildSystem::setMainFile(const QString &mainFilePath)
{
    if (m_projectItem)
        m_projectItem.data()->setMainFile(mainFilePath);
}

Utils::FilePath QmlBuildSystem::targetDirectory() const
{
    if (DeviceTypeKitAspect::deviceTypeId(target()->kit())
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
        return m_projectItem.data()->environment();
    return {};
}

QStringList QmlBuildSystem::customImportPaths() const
{
    if (m_projectItem)
        return m_projectItem.data()->importPaths();
    return {};
}

QStringList QmlBuildSystem::customFileSelectors() const
{
    if (m_projectItem)
        return m_projectItem.data()->fileSelectors();
    return {};
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

    const QtSupport::BaseQtVersion *version = QtSupport::QtKitAspect::qtVersion(k);
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
            if (version->qmlsceneCommand().isEmpty()) {
                result.append(createProjectTask(Task::TaskType::Error,
                                                tr("Qt version has no qmlscene command.")));
            }
        } else {
            // Non-desktop Qt on a desktop device? We don't support that.
            result.append(createProjectTask(Task::TaskType::Error,
                                            tr("Non-desktop Qt is used with a desktop device.")));
        }
    } else {
        // If not a desktop device, don't check the Qt version for qmlscene.
        // The device is responsible for providing it and we assume qmlscene can be found
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
        const QList<Kit*> kits = Utils::filtered(KitManager::kits(), [this](const Kit *k) {
            return !containsType(projectIssues(k), Task::TaskType::Error);
        });

        if (!kits.isEmpty()) {
            if (kits.contains(KitManager::defaultKit()))
                addTargetForDefaultKit();
            else
                addTargetForKit(kits.first());
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

    for (const QString &f : m_projectItem.data()->files()) {
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

    if (DeviceTypeKitAspect::deviceTypeId(target()->kit())
            == ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE) {
        return;
    }

    ProjectExplorer::DeploymentData deploymentData;
    for (const QString &file : m_projectItem->files()) {
        deploymentData.addFile(
                    file,
                    targetFile(Utils::FilePath::fromString(file)).parentDir().toString());
    }

    setDeploymentData(deploymentData);
}

QVariant QmlBuildSystem::additionalData(Id id) const
{
    if (id == Constants::customFileSelectorsData)
        return customFileSelectors();
    if (id == Constants::customForceFreeTypeData)
        return forceFreeType();
    if (id == Constants::customQtForMCUs)
        return qtForMCUs();
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
        return m_projectItem.data()->forceFreeType();
    return false;
}

bool QmlBuildSystem::addFiles(Node *context, const QStringList &filePaths, QStringList *)
{
    if (!dynamic_cast<QmlProjectNode *>(context))
        return false;

    QStringList toAdd;
    foreach (const QString &filePath, filePaths) {
        if (!m_projectItem.data()->matchesFile(filePath))
            toAdd << filePaths;
    }
    return toAdd.isEmpty();
}

bool QmlBuildSystem::deleteFiles(Node *context, const QStringList &filePaths)
{
    if (dynamic_cast<QmlProjectNode *>(context))
        return true;

    return BuildSystem::deleteFiles(context, filePaths);
}

bool QmlBuildSystem::renameFile(Node * context, const QString &filePath, const QString &newFilePath)
{
    if (dynamic_cast<QmlProjectNode *>(context)) {
        if (filePath.endsWith(mainFile())) {
            setMainFile(newFilePath);

            // make sure to change it also in the qmlproject file
            const QString qmlProjectFilePath = project()->projectFilePath().toString();
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
            QString originalFileName = QFileInfo(filePath).fileName();
            originalFileName.replace(".", "\\.");
            const QRegularExpression expression(QString("mainFile:\\s*\"(%1)\"").arg(originalFileName));
            const QRegularExpressionMatch match = expression.match(fileContent);

            fileContent.replace(match.capturedStart(1), match.capturedLength(1), QFileInfo(newFilePath).fileName());

            if (!textFileFormat.writeFile(qmlProjectFilePath, fileContent, &error))
                qWarning() << "Failed to write file" << qmlProjectFilePath << ":" << error;

            refresh(Everything);
        }

        return true;
    }

    return BuildSystem::renameFile(context, filePath, newFilePath);
}

} // namespace QmlProjectManager

