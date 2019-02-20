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

#include <resourceeditor/resourcenode.h>

#include <utils/qtcassert.h>
#include <utils/stringutils.h>

#include <android/androidconstants.h>

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>

using namespace ProjectExplorer;
using namespace Utils;

namespace QmakeProjectManager {

/*!
  \class QmakePriFileNode
  Implements abstract ProjectNode class
  */

QmakePriFileNode::QmakePriFileNode(QmakeProject *project, QmakeProFileNode *qmakeProFileNode,
                                   const FileName &filePath, QmakePriFile *pf) :
    ProjectNode(filePath),
    m_project(project),
    m_qmakeProFileNode(qmakeProFileNode),
    m_qmakePriFile(pf)
{ }

QmakePriFile *QmakePriFileNode::priFile() const
{
    if (!m_project->isParsing())
        return m_qmakePriFile;
    // During a parsing run the qmakePriFile tree will change, so search for the PriFile and
    // do not depend on the cached value.
    return m_project->rootProFile()->findPriFile(filePath());
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

bool QmakePriFileNode::supportsAction(ProjectAction action, const Node *node) const
{
    if (action == Rename || action == DuplicateFile) {
        const FileNode *fileNode = node->asFileNode();
        return (fileNode && fileNode->fileType() != FileType::Project)
                || dynamic_cast<const ResourceEditor::ResourceTopLevelNode *>(node);
    }

    const FolderNode *folderNode = this;
    const QmakeProFileNode *proFileNode;
    while (!(proFileNode = dynamic_cast<const QmakeProFileNode*>(folderNode))) {
        folderNode = folderNode->parentFolderNode();
        QTC_ASSERT(folderNode, return false);
    }
    QTC_ASSERT(proFileNode, return false);
    const QmakeProFile *pro = proFileNode->proFile();

    switch (pro ? pro->projectType() : ProjectType::Invalid) {
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
        if (node->nodeType() == NodeType::VirtualFolder) {
            // A virtual folder, we do what the projectexplorer does
            const FolderNode *folder = node->asFolderNode();
            if (folder) {
                QStringList list;
                foreach (FolderNode *f, folder->folderNodes())
                    list << f->filePath().toString() + QLatin1Char('/');
                if (deploysFolder(Utils::commonPath(list)))
                    addExistingFiles = false;
            }
        }

        addExistingFiles = addExistingFiles && !deploysFolder(node->filePath().toString());

        if (action == AddExistingFile || action == AddExistingDirectory)
            return addExistingFiles;

        break;
    }
    case ProjectType::SubDirsTemplate:
        if (action == AddSubProject)
            return true;
        break;
    default:
        break;
    }

    if (action == HasSubProjectRunConfigurations) {
        if (Target *t = m_project->activeTarget())  {
            auto canRunForNode = [node](RunConfiguration *rc) { return rc->canRunForNode(node); };
            if (Utils::anyOf(t->runConfigurations(), canRunForNode))
                return true;
        }
    }

    return false;
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

bool QmakePriFileNode::addFiles(const QStringList &filePaths, QStringList *notAdded)
{
    QmakePriFile *pri = priFile();
    if (!pri)
        return false;
    QList<Node *> matchingNodes = findNodes([filePaths](const Node *n) {
        return n->nodeType() == NodeType::File && filePaths.contains(n->filePath().toString());
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
    return pri->addFiles(actualFilePaths, notAdded);
}

bool QmakePriFileNode::removeFiles(const QStringList &filePaths, QStringList *notRemoved)
{
    QmakePriFile *pri = priFile();
    return pri ? pri->removeFiles(filePaths, notRemoved) : false;
}

bool QmakePriFileNode::deleteFiles(const QStringList &filePaths)
{
    QmakePriFile *pri = priFile();
    return pri ? pri->deleteFiles(filePaths) : false;
}

bool QmakePriFileNode::canRenameFile(const QString &filePath, const QString &newFilePath)
{
    QmakePriFile *pri = priFile();
    return pri ? pri->canRenameFile(filePath, newFilePath) : false;
}

bool QmakePriFileNode::renameFile(const QString &filePath, const QString &newFilePath)
{
    QmakePriFile *pri = priFile();
    return pri ? pri->renameFile(filePath, newFilePath) : false;
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
QmakeProFileNode::QmakeProFileNode(QmakeProject *project, const FileName &filePath, QmakeProFile *pf) :
    QmakePriFileNode(project, this, filePath, pf)
{ }

bool QmakeProFileNode::showInSimpleTree() const
{
    return showInSimpleTree(projectType()) || m_project->rootProjectNode() == this;
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

QVariant QmakeProFileNode::data(Core::Id role) const
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
        Utils::FileName destDir = info.destDir;
        if (!destDir.isEmpty()) {
            if (destDir.toFileInfo().isRelative())
                destDir = Utils::FileName::fromString(QDir::cleanPath(info.buildDir.toString()
                                                                      + '/' + destDir.toString()));
            res.append(destDir.toString());
        }
        res.removeDuplicates();
        return res;
    }
    QTC_CHECK(false);
    return {};
}

bool QmakeProFileNode::setData(Core::Id role, const QVariant &value) const
{
    QmakeProFile *pro = proFile();
    if (!pro)
        return false;

    const QString arch = pro->singleVariableValue(Variable::AndroidArch);
    const QString scope = "contains(ANDROID_TARGET_ARCH," + arch + ')';
    auto flags = QmakeProjectManager::Internal::ProWriter::ReplaceValues
               | QmakeProjectManager::Internal::ProWriter::MultiLine;

    if (role == Android::Constants::AndroidExtraLibs)
        return pro->setProVariable("ANDROID_EXTRA_LIBS", value.toStringList(), scope, flags);
    if (role == Android::Constants::AndroidPackageSourceDir)
        return pro->setProVariable("ANDROID_PACKAGE_SOURCE_DIR", {value.toString()}, scope, flags);

    return false;
}

QmakeProFile *QmakeProFileNode::proFile() const
{
    return static_cast<QmakeProFile*>(QmakePriFileNode::priFile());
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

bool QmakeProFileNode::supportsAction(ProjectAction action, const Node *node) const
{
    if (action == RemoveSubProject)
        return parentProjectNode() && !parentProjectNode()->asContainerNode();
    return QmakePriFileNode::supportsAction(action, node);
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

QString QmakeProFileNode::buildDir() const
{
    if (Target *target = m_project->activeTarget()) {
        if (BuildConfiguration *bc = target->activeBuildConfiguration()) {
            const QDir srcDirRoot(m_project->projectDirectory().toString());
            const QString relativeDir = srcDirRoot.relativeFilePath(filePath().parentDir().toString());
            return QDir::cleanPath(QDir(bc->buildDirectory().toString()).absoluteFilePath(relativeDir));
        }
    }
    return QString();
}

FileName QmakeProFileNode::buildDir(QmakeBuildConfiguration *bc) const
{
    const QmakeProFile *pro = proFile();
    return pro ? pro->buildDir(bc) : FileName();
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
