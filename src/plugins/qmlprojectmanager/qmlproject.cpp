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

#include <coreplugin/icontext.h>
#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>
#include <coreplugin/documentmanager.h>

#include <projectexplorer/deploymentdata.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/target.h>

#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtkitinformation.h>
#include <qtsupport/qtsupportconstants.h>
#include <qtsupport/desktopqtversion.h>
#include <qmljs/qmljsmodelmanagerinterface.h>

#include <utils/algorithm.h>

#include <QDebug>

using namespace Core;
using namespace ProjectExplorer;

namespace QmlProjectManager {

QmlProject::QmlProject(const Utils::FileName &fileName) :
    Project(QString::fromLatin1(Constants::QMLPROJECT_MIMETYPE), fileName,
            [this]() { refreshProjectFile(); })
{
    const QString normalized
            = Utils::FileUtils::normalizePathName(fileName.toFileInfo().canonicalFilePath());
    m_canonicalProjectDir = Utils::FileName::fromString(normalized).parentDir();

    setId(QmlProjectManager::Constants::QML_PROJECT_ID);
    setProjectLanguages(Context(ProjectExplorer::Constants::QMLJS_LANGUAGE_ID));
    setDisplayName(fileName.toFileInfo().completeBaseName());
}

QmlProject::~QmlProject()
{
    delete m_projectItem.data();
}

void QmlProject::addedTarget(Target *target)
{
    updateDeploymentData(target);
}

void QmlProject::onActiveTargetChanged(Target *target)
{
    if (m_activeTarget)
        disconnect(m_activeTarget, &Target::kitChanged, this, &QmlProject::onKitChanged);
    m_activeTarget = target;
    if (m_activeTarget)
        connect(target, &Target::kitChanged, this, &QmlProject::onKitChanged);

    // make sure e.g. the default qml imports are adapted
    refresh(Configuration);
}

void QmlProject::onKitChanged()
{
    // make sure e.g. the default qml imports are adapted
    refresh(Configuration);
}

Utils::FileName QmlProject::canonicalProjectDir() const
{
    return m_canonicalProjectDir;
}

void QmlProject::parseProject(RefreshOptions options)
{
    if (options & Files) {
        if (options & ProjectFile)
            delete m_projectItem.data();
        if (!m_projectItem) {
              QString errorMessage;
              m_projectItem = QmlProjectFileFormat::parseProjectFile(projectFilePath(), &errorMessage);
              if (m_projectItem) {
                  connect(m_projectItem.data(), &QmlProjectItem::qmlFilesChanged,
                          this, &QmlProject::refreshFiles);

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

void QmlProject::refresh(RefreshOptions options)
{
    emitParsingStarted();
    parseProject(options);

    if (options & Files)
        generateProjectTree();

    auto modelManager = QmlJS::ModelManagerInterface::instance();
    if (!modelManager)
        return;

    QmlJS::ModelManagerInterface::ProjectInfo projectInfo =
            modelManager->defaultProjectInfoForProject(this);
    foreach (const QString &searchPath, makeAbsolute(canonicalProjectDir(), customImportPaths()))
        projectInfo.importPaths.maybeInsert(Utils::FileName::fromString(searchPath),
                                            QmlJS::Dialect::Qml);

    modelManager->updateProjectInfo(projectInfo, this);

    emitParsingFinished(true);
}

QString QmlProject::mainFile() const
{
    if (m_projectItem)
        return m_projectItem.data()->mainFile();
    return QString();
}

void QmlProject::setMainFile(const QString &mainFilePath)
{
    if (m_projectItem)
        m_projectItem.data()->setMainFile(mainFilePath);
}

Utils::FileName QmlProject::targetDirectory(const Target *target) const
{
    if (DeviceTypeKitInformation::deviceTypeId(target->kit())
            == ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE)
        return canonicalProjectDir();

    return m_projectItem ? Utils::FileName::fromString(m_projectItem->targetDirectory())
                         : Utils::FileName();
}

Utils::FileName QmlProject::targetFile(const Utils::FileName &sourceFile,
                                       const Target *target) const
{
    const QDir sourceDir(m_projectItem ? m_projectItem->sourceDirectory()
                                       : canonicalProjectDir().toString());
    const QDir targetDir(targetDirectory(target).toString());
    const QString relative = sourceDir.relativeFilePath(sourceFile.toString());
    return Utils::FileName::fromString(QDir::cleanPath(targetDir.absoluteFilePath(relative)));
}

QList<Utils::EnvironmentItem> QmlProject::environment() const
{
    if (m_projectItem)
        return m_projectItem.data()->environment();
    return {};
}

bool QmlProject::validProjectFile() const
{
    return !m_projectItem.isNull();
}

QStringList QmlProject::customImportPaths() const
{
    if (m_projectItem)
        return m_projectItem.data()->importPaths();
    return {};
}

bool QmlProject::addFiles(const QStringList &filePaths)
{
    QStringList toAdd;
    foreach (const QString &filePath, filePaths) {
        if (!m_projectItem.data()->matchesFile(filePath))
            toAdd << filePaths;
    }
    return toAdd.isEmpty();
}

void QmlProject::refreshProjectFile()
{
    refresh(QmlProject::ProjectFile | Files);
}

bool QmlProject::needsBuildConfigurations() const
{
    return false;
}

QStringList QmlProject::makeAbsolute(const Utils::FileName &path, const QStringList &relativePaths)
{
    if (path.isEmpty())
        return relativePaths;

    const QDir baseDir(path.toString());
    return Utils::transform(relativePaths, [&baseDir](const QString &path) {
        return QDir::cleanPath(baseDir.absoluteFilePath(path));
    });
}

void QmlProject::refreshFiles(const QSet<QString> &/*added*/, const QSet<QString> &removed)
{
    refresh(Files);
    if (!removed.isEmpty()) {
        if (auto modelManager = QmlJS::ModelManagerInterface::instance())
            modelManager->removeFiles(removed.toList());
    }
    refreshTargetDirectory();
}

void QmlProject::refreshTargetDirectory()
{
    const QList<Target *> targetList = targets();
    for (Target *target : targetList)
        updateDeploymentData(target);
}

QList<Task> QmlProject::projectIssues(const Kit *k) const
{
    QList<Task> result = Project::projectIssues(k);

    const QtSupport::BaseQtVersion *version = QtSupport::QtKitInformation::qtVersion(k);
    if (!version)
        result.append(createProjectTask(Task::TaskType::Error, tr("No Qt version set in kit.")));

    IDevice::ConstPtr dev = DeviceKitInformation::device(k);
    if (dev.isNull())
        result.append(createProjectTask(Task::TaskType::Error, tr("Kit has no device.")));

    if (version && version->qtVersion() < QtSupport::QtVersionNumber(5, 0, 0))
        result.append(createProjectTask(Task::TaskType::Error, tr("Qt version is too old.")));

    if (dev.isNull() || !version)
        return result; // No need to check deeper than this

    if (dev->type() == ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE) {
        if (version->type() == QtSupport::Constants::DESKTOPQT) {
            if (static_cast<const QtSupport::DesktopQtVersion *>(version)->qmlsceneCommand().isEmpty()) {
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

    // refresh first - project information is used e.g. to decide the default RC's
    refresh(Everything);

    if (!activeTarget()) {
        // find a kit that matches prerequisites (prefer default one)
        const QList<Kit*> kits = KitManager::kits([this](const Kit *k) {
            return !containsType(projectIssues(k), Task::TaskType::Error);
        });

        if (!kits.isEmpty()) {
            Kit *kit = kits.contains(KitManager::defaultKit()) ? KitManager::defaultKit() : kits.first();
            addTarget(createTarget(kit));
        }
    }

    // addedTarget calls updateEnabled on the runconfigurations
    // which needs to happen after refresh
    foreach (Target *t, targets())
        addedTarget(t);

    connect(this, &ProjectExplorer::Project::addedTarget, this, &QmlProject::addedTarget);

    connect(this, &ProjectExplorer::Project::activeTargetChanged,
            this, &QmlProject::onActiveTargetChanged);

    onActiveTargetChanged(activeTarget());

    return RestoreResult::Ok;
}

void QmlProject::generateProjectTree()
{
    if (!m_projectItem)
        return;

    auto newRoot = std::make_unique<Internal::QmlProjectNode>(this);

    for (const QString &f : m_projectItem.data()->files()) {
        const Utils::FileName fileName = Utils::FileName::fromString(f);
        const FileType fileType = (fileName == projectFilePath())
                ? FileType::Project : FileNode::fileTypeForFileName(fileName);
        newRoot->addNestedNode(std::make_unique<FileNode>(fileName, fileType, false));
    }
    newRoot->addNestedNode(std::make_unique<FileNode>(projectFilePath(), FileType::Project, false));

    setRootProjectNode(std::move(newRoot));
    refreshTargetDirectory();
}

void QmlProject::updateDeploymentData(ProjectExplorer::Target *target)
{
    if (!m_projectItem)
        return;

    if (DeviceTypeKitInformation::deviceTypeId(target->kit())
            == ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE) {
        return;
    }

    ProjectExplorer::DeploymentData deploymentData;
    for (const QString &file : m_projectItem->files()) {
        deploymentData.addFile(
                    file,
                    targetFile(Utils::FileName::fromString(file), target).parentDir().toString());
    }

    target->setDeploymentData(deploymentData);
}
} // namespace QmlProjectManager

