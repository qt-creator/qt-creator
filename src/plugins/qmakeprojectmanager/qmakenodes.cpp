// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmakenodes.h"

#include "qmakeproject.h"

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/target.h>

#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtkitaspect.h>

#include <resourceeditor/resourcenode.h>

#include <utils/fileutils.h>
#include <utils/qtcassert.h>

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
                    FilePaths list;
                    folder->forEachFolderNode([&](FolderNode *f) { list << f->filePath(); });
                    if (n->deploysFolder(FileUtils::commonPath(list).toString()))
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

bool QmakePriFileNode::canAddSubProject(const FilePath &proFilePath) const
{
    const QmakePriFile *pri = priFile();
    return pri ? pri->canAddSubProject(proFilePath) : false;
}

bool QmakePriFileNode::addSubProject(const FilePath &proFilePath)
{
    QmakePriFile *pri = priFile();
    return pri ? pri->addSubProject(proFilePath) : false;
}

bool QmakePriFileNode::removeSubProject(const FilePath &proFilePath)
{
    QmakePriFile *pri = priFile();
    return pri ? pri->removeSubProjects(proFilePath) : false;
}

QStringList QmakePriFileNode::subProjectFileNamePatterns() const
{
    return {"*.pro"};
}

bool QmakeBuildSystem::addFiles(Node *context, const FilePaths &filePaths, FilePaths *notAdded)
{
    if (auto n = dynamic_cast<QmakePriFileNode *>(context)) {
        QmakePriFile *pri = n->priFile();
        if (!pri)
            return false;
        QList<Node *> matchingNodes = n->findNodes([filePaths](const Node *nn) {
            return nn->asFileNode() && filePaths.contains(nn->filePath());
        });
        matchingNodes = filtered(matchingNodes, [](const Node *n) {
            for (const Node *parent = n->parentFolderNode(); parent;
                 parent = parent->parentFolderNode()) {
                if (dynamic_cast<const ResourceEditor::ResourceTopLevelNode *>(parent))
                    return false;
            }
            return true;
        });
        FilePaths alreadyPresentFiles = transform(matchingNodes, [](const Node *n) { return n->filePath(); });
        FilePath::removeDuplicates(alreadyPresentFiles);

        FilePaths actualFilePaths = filePaths;
        for (const FilePath &e : alreadyPresentFiles)
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

RemovedFilesFromProject QmakeBuildSystem::removeFiles(Node *context, const FilePaths &filePaths,
                                                      FilePaths *notRemoved)
{
    if (auto n = dynamic_cast<QmakePriFileNode *>(context)) {
        QmakePriFile * const pri = n->priFile();
        if (!pri)
            return RemovedFilesFromProject::Error;
        FilePaths wildcardFiles;
        FilePaths nonWildcardFiles;
        for (const FilePath &file : filePaths) {
            if (pri->proFile()->isFileFromWildcard(file.toString()))
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

bool QmakeBuildSystem::deleteFiles(Node *context, const FilePaths &filePaths)
{
    if (auto n = dynamic_cast<QmakePriFileNode *>(context)) {
        QmakePriFile *pri = n->priFile();
        return pri ? pri->deleteFiles(filePaths) : false;
    }

    return BuildSystem::deleteFiles(context, filePaths);
}

bool QmakeBuildSystem::canRenameFile(Node *context,
                                     const FilePath &oldFilePath,
                                     const FilePath &newFilePath)
{
    if (auto n = dynamic_cast<QmakePriFileNode *>(context)) {
        QmakePriFile *pri = n->priFile();
        return pri ? pri->canRenameFile(oldFilePath, newFilePath) : false;
    }

    return BuildSystem::canRenameFile(context, oldFilePath, newFilePath);
}

bool QmakeBuildSystem::renameFile(Node *context,
                                  const FilePath &oldFilePath,
                                  const FilePath &newFilePath)
{
    if (auto n = dynamic_cast<QmakePriFileNode *>(context)) {
        QmakePriFile *pri = n->priFile();
        return pri ? pri->renameFile(oldFilePath, newFilePath) : false;
    }

    return BuildSystem::renameFile(context, oldFilePath, newFilePath);
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

FolderNode::AddNewInformation QmakePriFileNode::addNewInformation(const FilePaths &files, Node *context) const
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

QVariant QmakeProFileNode::data(Id role) const
{
    if (role == Android::Constants::AndroidAbis)
        return variableValue(Variable::AndroidAbis);
    if (role == Android::Constants::AndroidAbi)
        return singleVariableValue(Variable::AndroidAbi);
    if (role == Android::Constants::AndroidExtraLibs)
        return variableValue(Variable::AndroidExtraLibs);
    if (role == Android::Constants::AndroidPackageSourceDir)
        return singleVariableValue(Variable::AndroidPackageSourceDir);
    if (role == Android::Constants::AndroidDeploySettingsFile)
        return singleVariableValue(Variable::AndroidDeploySettingsFile);
    if (role == Android::Constants::AndroidSoLibPath) {
        TargetInformation info = targetInformation();
        QStringList res = {info.buildDir.toString()};
        FilePath destDir = info.destDir;
        if (!destDir.isEmpty()) {
            destDir = info.buildDir.resolvePath(destDir.path());
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

    if (role == Ios::Constants::IosCmakeGenerator) {
        // qmake is not CMake, so return empty value
        return {};
    }

    if (role == ProjectExplorer::Constants::QT_KEYWORDS_ENABLED)
        return !proFile()->variableValue(Variable::Config).contains("no_keywords");

    QTC_CHECK(false);
    return {};
}

bool QmakeProFileNode::setData(Id role, const QVariant &value) const
{
    QmakeProFile *pro = proFile();
    if (!pro)
        return false;
    QString scope;
    int flags = QmakeProjectManager::Internal::ProWriter::ReplaceValues;
    if (Target *target = m_buildSystem->target()) {
        QtSupport::QtVersion *version = QtSupport::QtKitAspect::qtVersion(target->kit());
        if (version && !version->supportsMultipleQtAbis()) {
            const QString arch = pro->singleVariableValue(Variable::AndroidAbi);
            scope = QString("contains(%1,%2)").arg(Android::Constants::ANDROID_TARGET_ARCH)
                                              .arg(arch);
            flags |= QmakeProjectManager::Internal::ProWriter::MultiLine;
        }
    }

    if (role == Android::Constants::AndroidExtraLibs)
        return pro->setProVariable(QLatin1String(Android::Constants::ANDROID_EXTRA_LIBS),
                                   value.toStringList(), scope, flags);
    if (role == Android::Constants::AndroidPackageSourceDir)
        return pro->setProVariable(QLatin1String(Android::Constants::ANDROID_PACKAGE_SOURCE_DIR),
                                   {value.toString()}, scope, flags);
    if (role == Android::Constants::AndroidApplicationArgs)
        return pro->setProVariable(QLatin1String(Android::Constants::ANDROID_APPLICATION_ARGUMENTS),
                                   {value.toString()}, scope, flags);

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

FolderNode::AddNewInformation QmakeProFileNode::addNewInformation(const FilePaths &files, Node *context) const
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
