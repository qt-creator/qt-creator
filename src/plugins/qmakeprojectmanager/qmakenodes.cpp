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

#include "qmakenodes.h"

#include "qmakeproject.h"

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/target.h>

#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtkitinformation.h>

#include <resourceeditor/resourcenode.h>

#include <utils/qtcassert.h>
#include <utils/stringutils.h>

#include <android/androidconstants.h>
#include <ios/iosconstants.h>

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>

using namespace ProjectExplorer;
using namespace Utils;

using namespace QmakeProjectManager::Internal;

namespace QmakeProjectManager {

/*!
  \class QmakePriFileNode
  Implements abstract ProjectNode class
  */

QmakePriFileNode::QmakePriFileNode(QmakeBuildSystem *buildSystem, QmakeProFileNode *qmakeProFileNode,
                                   const FilePath &filePath, QmakePriFile *pf) :
    ProjectNode(filePath),
    m_buildSystem(buildSystem),
    m_qmakeProFileNode(qmakeProFileNode),
    m_qmakePriFile(pf)
{ }

QmakePriFile *QmakePriFileNode::priFile() const
{
    if (!m_buildSystem)
        return nullptr;

    if (!m_buildSystem->isParsing())
        return m_qmakePriFile;

    // During a parsing run the qmakePriFile tree will change, so search for the PriFile and
    // do not depend on the cached value.
    // NOTE: This would go away if the node tree would be per-buildsystem
    return m_buildSystem->rootProFile()->findPriFile(filePath());
}

bool QmakePriFileNode::deploysFolder(const QString &folder) const
{
    const QmakePriFile *pri = priFile();
    return pri ? pri->deploysFolder(folder) : false;
}

QmakeProFileNode *QmakePriFileNode::proFileNode() const
{
    return m_qmakeProFileNode;
}

bool QmakeBuildSystem::supportsAction(Node *context, ProjectAction action, const Node *node) const
{
    if (auto n = dynamic_cast<QmakePriFileNode *>(context)) { // Covers QmakeProfile, too.
        if (action == Rename) {
            const FileNode *fileNode = node->asFileNode();
            return (fileNode && fileNode->fileType() != FileType::Project)
                    || dynamic_cast<const ResourceEditor::ResourceTopLevelNode *>(node);
        }

        ProjectType t = ProjectType::Invalid;
        const QmakeProFile *pro = nullptr;
        if (hasParsingData()) {
            const FolderNode *folderNode = n;
            const QmakeProFileNode *proFileNode;
            while (!(proFileNode = dynamic_cast<const QmakeProFileNode*>(folderNode))) {
                folderNode = folderNode->parentFolderNode();
                QTC_ASSERT(folderNode, return false);
            }
            QTC_ASSERT(proFileNode, return false);
            pro = proFileNode->proFile();
            QTC_ASSERT(pro, return false);
            t = pro->projectType();
        }

        switch (t) {
        case ProjectType::ApplicationTemplate:
        case ProjectType::StaticLibraryTemplate:
        case ProjectType::SharedLibraryTemplate:
        case ProjectType::AuxTemplate: {
            // TODO: Some of the file types don't make much sense for aux
            // projects (e.g. cpp). It'd be nice if the "add" action could
            // work on a subset of the file types according to project type.
            if (action == AddNewFile)
                return true;
            if (action == EraseFile)
                return pro && pro->knowsFile(node->filePath());
            if (action == RemoveFile)
                return !(pro && pro->knowsFile(node->filePath()));

            bool addExistingFiles = true;
            if (node->isVirtualFolderType()) {
                // A virtual folder, we do what the projectexplorer does
                const FolderNode *folder = node->asFolderNode();
                if (folder) {
                    QStringList list;
                    foreach (FolderNode *f, folder->folderNodes())
                        list << f->filePath().toString() + QLatin1Char('/');
                    if (n->deploysFolder(Utils::commonPath(list)))
                        addExistingFiles = false;
                }
            }

            addExistingFiles = addExistingFiles && !n->deploysFolder(node->filePath().toString());

            if (action == AddExistingFile || action == AddExistingDirectory)
                return addExistingFiles;

            break;
        }
        case ProjectType::SubDirsTemplate:
            if (action == AddSubProject || action == AddExistingProject)
                return true;
            break;
        default:
            break;
        }

        return false;
    }

    if (auto n = dynamic_cast<QmakeProFileNode *>(context)) {
        if (action == RemoveSubProject)
            return n->parentProjectNode() && !n->parentProjectNode()->asContainerNode();
    }

    return BuildSystem::supportsAction(context, action, node);
}

bool QmakePriFileNode::canAddSubProject(const QString &proFilePath) const
{
    const QmakePriFile *pri = priFile();
    return pri ? pri->canAddSubProject(proFilePath) : false;
}

bool QmakePriFileNode::addSubProject(const QString &proFilePath)
{
    QmakePriFile *pri = priFile();
    return pri ? pri->addSubProject(proFilePath) : false;
}

bool QmakePriFileNode::removeSubProject(const QString &proFilePath)
{
    QmakePriFile *pri = priFile();
    return pri ? pri->removeSubProjects(proFilePath) : false;
}

QStringList QmakePriFileNode::subProjectFileNamePatterns() const
{
    return QStringList("*.pro");
}

bool QmakeBuildSystem::addFiles(Node *context, const QStringList &filePaths, QStringList *notAdded)
{
    if (auto n = dynamic_cast<QmakePriFileNode *>(context)) {
        QmakePriFile *pri = n->priFile();
        if (!pri)
            return false;
        QList<Node *> matchingNodes = n->findNodes([filePaths](const Node *nn) {
            return nn->asFileNode() && filePaths.contains(nn->filePath().toString());
        });
        matchingNodes = filtered(matchingNodes, [](const Node *n) {
            for (const Node *parent = n->parentFolderNode(); parent;
                 parent = parent->parentFolderNode()) {
                if (dynamic_cast<const ResourceEditor::ResourceTopLevelNode *>(parent))
                    return false;
            }
            return true;
        });
        QStringList alreadyPresentFiles = transform<QStringList>(matchingNodes,
                                                                 [](const Node *n) { return n->filePath().toString(); });
        alreadyPresentFiles.removeDuplicates();
        QStringList actualFilePaths = filePaths;
        for (const QString &e : alreadyPresentFiles)
            actualFilePaths.removeOne(e);
        if (notAdded)
            *notAdded = alreadyPresentFiles;
        qCDebug(qmakeNodesLog) << Q_FUNC_INFO << "file paths:" << filePaths
                               << "already present:" << alreadyPresentFiles
                               << "actual file paths:" << actualFilePaths;
        return pri->addFiles(actualFilePaths, notAdded);
    }

    return BuildSystem::addFiles(context, filePaths, notAdded);
}

RemovedFilesFromProject QmakeBuildSystem::removeFiles(Node *context, const QStringList &filePaths,
                                                      QStringList *notRemoved)
{
    if (auto n = dynamic_cast<QmakePriFileNode *>(context)) {
        QmakePriFile * const pri = n->priFile();
        if (!pri)
            return RemovedFilesFromProject::Error;
        QStringList wildcardFiles;
        QStringList nonWildcardFiles;
        for (const QString &file : filePaths) {
            if (pri->proFile()->isFileFromWildcard(file))
                wildcardFiles << file;
            else
                nonWildcardFiles << file;
        }
        const bool success = pri->removeFiles(nonWildcardFiles, notRemoved);
        if (notRemoved)
            *notRemoved += wildcardFiles;
        if (!success)
            return RemovedFilesFromProject::Error;
        if (!wildcardFiles.isEmpty())
            return RemovedFilesFromProject::Wildcard;
        return RemovedFilesFromProject::Ok;
    }

    return BuildSystem::removeFiles(context, filePaths, notRemoved);
}

bool QmakeBuildSystem::deleteFiles(Node *context, const QStringList &filePaths)
{
    if (auto n = dynamic_cast<QmakePriFileNode *>(context)) {
        QmakePriFile *pri = n->priFile();
        return pri ? pri->deleteFiles(filePaths) : false;
    }

    return BuildSystem::deleteFiles(context, filePaths);
}

bool QmakeBuildSystem::canRenameFile(Node *context, const QString &filePath, const QString &newFilePath)
{
    if (auto n = dynamic_cast<QmakePriFileNode *>(context)) {
        QmakePriFile *pri = n->priFile();
        return pri ? pri->canRenameFile(filePath, newFilePath) : false;
    }

    return BuildSystem::canRenameFile(context, filePath, newFilePath);
}

bool QmakeBuildSystem::renameFile(Node *context, const QString &filePath, const QString &newFilePath)
{
    if (auto n = dynamic_cast<QmakePriFileNode *>(context)) {
        QmakePriFile *pri = n->priFile();
        return pri ? pri->renameFile(filePath, newFilePath) : false;
    }

    return BuildSystem::renameFile(context, filePath, newFilePath);
}

bool QmakeBuildSystem::addDependencies(Node *context, const QStringList &dependencies)
{
    if (auto n = dynamic_cast<QmakePriFileNode *>(context)) {
        if (QmakePriFile * const pri = n->priFile())
            return pri->addDependencies(dependencies);
        return false;
    }

    return BuildSystem::addDependencies(context, dependencies);
}

FolderNode::AddNewInformation QmakePriFileNode::addNewInformation(const QStringList &files, Node *context) const
{
    Q_UNUSED(files)
    return FolderNode::AddNewInformation(filePath().fileName(), context && context->parentProjectNode() == this ? 120 : 90);
}

/*!
  \class QmakeProFileNode
  Implements abstract ProjectNode class
  */
QmakeProFileNode::QmakeProFileNode(QmakeBuildSystem *buildSystem, const FilePath &filePath, QmakeProFile *pf) :
    QmakePriFileNode(buildSystem, this, filePath, pf)
{
    if (projectType() == ProjectType::ApplicationTemplate) {
        setProductType(ProductType::App);
    } else if (projectType() == ProjectType::SharedLibraryTemplate
               || projectType() == ProjectType::StaticLibraryTemplate) {
        setProductType(ProductType::Lib);
    } else if (projectType() != ProjectType::SubDirsTemplate) {
        setProductType(ProductType::Other);
    }
}

bool QmakeProFileNode::showInSimpleTree() const
{
    return showInSimpleTree(projectType()) || m_buildSystem->project()->rootProjectNode() == this;
}

QString QmakeProFileNode::buildKey() const
{
    return filePath().toString();
}

bool QmakeProFileNode::parseInProgress() const
{
    QmakeProjectManager::QmakeProFile *pro = proFile();
    return !pro || pro->parseInProgress();
}

bool QmakeProFileNode::validParse() const
{
    QmakeProjectManager::QmakeProFile *pro = proFile();
    return pro && pro->validParse();
}

void QmakeProFileNode::build()
{
    m_buildSystem->buildHelper(QmakeBuildSystem::BUILD, false, this, nullptr);
}

QStringList QmakeProFileNode::targetApplications() const
{
    QStringList apps;
    if (includedInExactParse() && projectType() == ProjectType::ApplicationTemplate) {
        const QString target = targetInformation().target;
        if (target.startsWith("lib") && target.endsWith(".so"))
            apps << target.mid(3, target.lastIndexOf('.') - 3);
        else
            apps << target;
    }
    return apps;
}

QVariant QmakeProFileNode::data(Utils::Id role) const
{
    if (role == Android::Constants::AndroidPackageSourceDir)
        return singleVariableValue(Variable::AndroidPackageSourceDir);
    if (role == Android::Constants::AndroidDeploySettingsFile)
        return singleVariableValue(Variable::AndroidDeploySettingsFile);
    if (role == Android::Constants::AndroidExtraLibs)
        return variableValue(Variable::AndroidExtraLibs);
    if (role == Android::Constants::AndroidArch)
        return singleVariableValue(Variable::AndroidArch);
    if (role == Android::Constants::AndroidSoLibPath) {
        TargetInformation info = targetInformation();
        QStringList res = {info.buildDir.toString()};
        Utils::FilePath destDir = info.destDir;
        if (!destDir.isEmpty()) {
            if (destDir.toFileInfo().isRelative())
                destDir = Utils::FilePath::fromString(QDir::cleanPath(info.buildDir.toString()
                                                                      + '/' + destDir.toString()));
            res.append(destDir.toString());
        }
        res.removeDuplicates();
        return res;
    }

    if (role == Android::Constants::AndroidTargets)
        return {};
    if (role == Android::Constants::AndroidApk)
        return {};

    // We can not use AppMan headers even at build time.
    if (role == "AppmanPackageDir")
        return singleVariableValue(Variable::AppmanPackageDir);
    if (role == "AppmanManifest")
        return singleVariableValue(Variable::AppmanManifest);

    if (role == Ios::Constants::IosTarget) {
        const TargetInformation info = targetInformation();
        if (info.valid)
            return info.target;
    }

    if (role == Ios::Constants::IosBuildDir) {
        const TargetInformation info = targetInformation();
        if (info.valid)
            return info.buildDir.toString();
    }

    if (role == ProjectExplorer::Constants::QT_KEYWORDS_ENABLED)
        return !proFile()->variableValue(Variable::Config).contains("no_keywords");

    QTC_CHECK(false);
    return {};
}

bool QmakeProFileNode::setData(Utils::Id role, const QVariant &value) const
{
    QmakeProFile *pro = proFile();
    if (!pro)
        return false;
    QString scope;
    int flags = QmakeProjectManager::Internal::ProWriter::ReplaceValues;
    if (Target *target = m_buildSystem->target()) {
        QtSupport::BaseQtVersion *version = QtSupport::QtKitAspect::qtVersion(target->kit());
        if (version && version->qtVersion() < QtSupport::QtVersionNumber(5, 14, 0)) {
            const QString arch = pro->singleVariableValue(Variable::AndroidArch);
            scope = "contains(ANDROID_TARGET_ARCH," + arch + ')';
            flags |= QmakeProjectManager::Internal::ProWriter::MultiLine;
        }
    }

    if (role == Android::Constants::AndroidExtraLibs)
        return pro->setProVariable("ANDROID_EXTRA_LIBS", value.toStringList(), scope, flags);
    if (role == Android::Constants::AndroidPackageSourceDir)
        return pro->setProVariable("ANDROID_PACKAGE_SOURCE_DIR", {value.toString()}, scope, flags);

    return false;
}

QmakeProFile *QmakeProFileNode::proFile() const
{
    return dynamic_cast<QmakeProFile*>(QmakePriFileNode::priFile());
}

QString QmakeProFileNode::makefile() const
{
    return singleVariableValue(Variable::Makefile);
}

QString QmakeProFileNode::objectsDirectory() const
{
    return singleVariableValue(Variable::ObjectsDir);
}

bool QmakeProFileNode::isDebugAndRelease() const
{
    const QStringList configValues = variableValue(Variable::Config);
    return configValues.contains(QLatin1String("debug_and_release"));
}

bool QmakeProFileNode::isObjectParallelToSource() const
{
    return variableValue(Variable::Config).contains("object_parallel_to_source");
}

bool QmakeProFileNode::isQtcRunnable() const
{
    const QStringList configValues = variableValue(Variable::Config);
    return configValues.contains(QLatin1String("qtc_runnable"));
}

bool QmakeProFileNode::includedInExactParse() const
{
    const QmakeProFile *pro = proFile();
    return pro && pro->includedInExactParse();
}

FolderNode::AddNewInformation QmakeProFileNode::addNewInformation(const QStringList &files, Node *context) const
{
    Q_UNUSED(files)
    return AddNewInformation(filePath().fileName(), context && context->parentProjectNode() == this ? 120 : 100);
}

bool QmakeProFileNode::showInSimpleTree(ProjectType projectType) const
{
    return projectType == ProjectType::ApplicationTemplate
            || projectType == ProjectType::SharedLibraryTemplate
            || projectType == ProjectType::StaticLibraryTemplate;
}

ProjectType QmakeProFileNode::projectType() const
{
    const QmakeProFile *pro = proFile();
    return pro ? pro->projectType() : ProjectType::Invalid;
}

QStringList QmakeProFileNode::variableValue(const Variable var) const
{
    QmakeProFile *pro = proFile();
    return pro ? pro->variableValue(var) : QStringList();
}

QString QmakeProFileNode::singleVariableValue(const Variable var) const
{
    const QStringList &values = variableValue(var);
    return values.isEmpty() ? QString() : values.first();
}

QString QmakeProFileNode::objectExtension() const
{
    QStringList exts = variableValue(Variable::ObjectExt);
    if (exts.isEmpty())
        return HostOsInfo::isWindowsHost() ? QLatin1String(".obj") : QLatin1String(".o");
    return exts.first();
}

TargetInformation QmakeProFileNode::targetInformation() const
{
    return proFile() ? proFile()->targetInformation() : TargetInformation();
}

} // namespace QmakeProjectManager
