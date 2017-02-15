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

#include "qbsnodes.h"

#include "qbsproject.h"
#include "qbsprojectmanagerconstants.h"
#include "qbsrunconfiguration.h"

#include <coreplugin/fileiconprovider.h>
#include <coreplugin/idocument.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>
#include <qtsupport/qtsupportconstants.h>
#include <resourceeditor/resourcenode.h>
#include <utils/algorithm.h>
#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>

#include <QtDebug>
#include <QDir>
#include <QStyle>

// ----------------------------------------------------------------------
// Helpers:
// ----------------------------------------------------------------------

static QString displayNameFromPath(const QString &path, const QString &base)
{
    QString dir = base;
    if (!base.endsWith(QLatin1Char('/')))
        dir.append(QLatin1Char('/'));

    QString name = path;
    if (name.startsWith(dir)) {
        name = name.mid(dir.count());
    } else {
        QFileInfo fi = QFileInfo(path);
        name = QCoreApplication::translate("Qbs::QbsProjectNode", "%1 in %2")
                .arg(fi.fileName(), fi.absolutePath());
    }

    return name;
}

static QIcon generateIcon(const QString &overlay)
{
    const QSize desiredSize = QSize(16, 16);
    const QIcon overlayIcon(overlay);
    const QPixmap pixmap
            = Core::FileIconProvider::overlayIcon(QStyle::SP_DirIcon, overlayIcon, desiredSize);

    QIcon result;
    result.addPixmap(pixmap);

    return result;
}

namespace QbsProjectManager {
namespace Internal {

QIcon QbsGroupNode::m_groupIcon;
QIcon QbsProjectNode::m_projectIcon;
QIcon QbsProductNode::m_productIcon;

static QbsProjectNode *parentQbsProjectNode(ProjectExplorer::Node *node)
{
    for (ProjectExplorer::FolderNode *pn = node->managingProject(); pn; pn = pn->parentProjectNode()) {
        QbsProjectNode *prjNode = dynamic_cast<QbsProjectNode *>(pn);
        if (prjNode)
            return prjNode;
    }
    return 0;
}

static QbsProductNode *parentQbsProductNode(ProjectExplorer::Node *node)
{
    for (; node; node = node->parentFolderNode()) {
        QbsProductNode *prdNode = dynamic_cast<QbsProductNode *>(node);
        if (prdNode)
            return prdNode;
    }
    return 0;
}

static qbs::GroupData findMainQbsGroup(const qbs::ProductData &productData)
{
    foreach (const qbs::GroupData &grp, productData.groups()) {
        if (grp.name() == productData.name() && grp.location() == productData.location())
            return grp;
    }
    return qbs::GroupData();
}

class FileTreeNode {
public:
    explicit FileTreeNode(const QString &n = QString(), FileTreeNode *p = 0, bool f = false) :
        parent(p), name(n), m_isFile(f)
    {
        if (p)
            p->children.append(this);
    }

    ~FileTreeNode()
    {
        qDeleteAll(children);
    }

    FileTreeNode *addPart(const QString &n, bool isFile)
    {
        foreach (FileTreeNode *c, children) {
            if (c->name == n)
                return c;
        }
        return new FileTreeNode(n, this, isFile);
    }

    bool isFile() const { return m_isFile; }

    static FileTreeNode *moveChildrenUp(FileTreeNode *node)
    {
        QTC_ASSERT(node, return 0);

        FileTreeNode *newParent = node->parent;
        if (!newParent)
            return 0;

        // disconnect node and parent:
        node->parent = 0;
        newParent->children.removeOne(node);

        foreach (FileTreeNode *c, node->children) {
            // update path, make sure there will be no / before "C:" on windows:
            if (Utils::HostOsInfo::isWindowsHost() && node->name.isEmpty())
                c->name = node->name;
            else
                c->name = node->name + QLatin1Char('/') + c->name;

            newParent->children.append(c);
            c->parent = newParent;
        }

        // Delete node
        node->children.clear();
        delete node;
        return newParent;
    }

    // Moves the children of the node pointing to basedir to the root of the tree.
    static void reorder(FileTreeNode *node, const QString &basedir)
    {
        QTC_CHECK(!basedir.isEmpty());
        QString prefix = basedir;
        if (basedir.startsWith(QLatin1Char('/')))
            prefix = basedir.mid(1);
        prefix.append(QLatin1Char('/'));

        if (node->path() == basedir) {
            // Find root node:
            FileTreeNode *root = node;
            while (root->parent)
                root = root->parent;

            foreach (FileTreeNode *c, node->children) {
                // Update children names by prepending basedir
                c->name = prefix + c->name;
                // Update parent information:
                c->parent = root;

                root->children.append(c);
            }

            // Clean up node:
            node->children.clear();
            node->parent->children.removeOne(node);
            node->parent = 0;
            delete node;

            return;
        }

        foreach (FileTreeNode *n, node->children)
            reorder(n, basedir);
    }

    static void simplify(FileTreeNode *node)
    {
        foreach (FileTreeNode *c, node->children)
            simplify(c);

        if (!node->parent)
            return;

        if (node->children.isEmpty() && !node->isFile()) {
            // Clean up empty folder nodes:
            node->parent->children.removeOne(node);
            node->parent = 0;
            delete node;
        } else if (node->children.count() == 1 && !node->children.at(0)->isFile()) {
            // Compact folder nodes with one child only:
            moveChildrenUp(node);
        }
    }

    QString path() const
    {
        QString p = name;
        FileTreeNode *node = parent;
        while (node) {
            if (!Utils::HostOsInfo::isWindowsHost() || !node->name.isEmpty())
                p = node->name + QLatin1Char('/') + p;
            node = node->parent;
        }
        return p;
    }

    QList<FileTreeNode *> children;
    FileTreeNode *parent;
    QString name;
    bool m_isFile;
};


static QList<ProjectExplorer::ProjectAction> supportedNodeActions(ProjectExplorer::Node *node,
                                                                  bool managesFiles)
{
    QList<ProjectExplorer::ProjectAction> actions;
    const QbsProject * const project = parentQbsProjectNode(node)->project();
    if (!project->isProjectEditable())
        return actions;
    if (managesFiles)
        actions << ProjectExplorer::AddNewFile << ProjectExplorer::AddExistingFile;
    if (node->nodeType() == ProjectExplorer::NodeType::File
            && !project->qbsProject().buildSystemFiles().contains(node->filePath().toString())) {
        actions << ProjectExplorer::RemoveFile << ProjectExplorer::Rename;
    }
    return actions;
}

// ----------------------------------------------------------------------
// QbsFileNode:
// ----------------------------------------------------------------------

QbsFileNode::QbsFileNode(const Utils::FileName &filePath,
                         const ProjectExplorer::FileType fileType,
                         bool generated,
                         int line) :
    ProjectExplorer::FileNode(filePath, fileType, generated, line)
{ }

QString QbsFileNode::displayName() const
{
    int l = line();
    if (l < 0)
        return ProjectExplorer::FileNode::displayName();
    return ProjectExplorer::FileNode::displayName() + QLatin1Char(':') + QString::number(l);
}


QbsFolderNode::QbsFolderNode(const Utils::FileName &folderPath, ProjectExplorer::NodeType nodeType,
                             const QString &displayName, bool isGeneratedFilesFolder)
    : ProjectExplorer::FolderNode(folderPath, nodeType, displayName),
      m_isGeneratedFilesFolder(isGeneratedFilesFolder)
{
}

QList<ProjectExplorer::ProjectAction> QbsFolderNode::supportedActions(ProjectExplorer::Node *node) const
{
    return supportedNodeActions(node, false);
}

// ---------------------------------------------------------------------------
// QbsBaseProjectNode:
// ---------------------------------------------------------------------------

QbsBaseProjectNode::QbsBaseProjectNode(const Utils::FileName &path) :
    ProjectExplorer::ProjectNode(path)
{ }

bool QbsBaseProjectNode::showInSimpleTree() const
{
    return false;
}

QList<ProjectExplorer::ProjectAction> QbsBaseProjectNode::supportedActions(ProjectExplorer::Node *node) const
{
    Q_UNUSED(node);
    return QList<ProjectExplorer::ProjectAction>();
}

// --------------------------------------------------------------------
// QbsGroupNode:
// --------------------------------------------------------------------

QbsGroupNode::QbsGroupNode(const qbs::GroupData &grp, const QString &productPath) :
    QbsBaseProjectNode(Utils::FileName())
{
    if (m_groupIcon.isNull())
        m_groupIcon = QIcon(QString::fromLatin1(Constants::QBS_GROUP_ICON));

    setIcon(m_groupIcon);

    addNode(new QbsFileNode(Utils::FileName::fromString(grp.location().filePath()),
                            ProjectExplorer::FileType::Project, false,
                            grp.location().line()));

    updateQbsGroupData(grp, productPath, true);
}

QList<ProjectExplorer::ProjectAction> QbsGroupNode::supportedActions(ProjectExplorer::Node *node) const
{
    return supportedNodeActions(node, true);
}

bool QbsGroupNode::addFiles(const QStringList &filePaths, QStringList *notAdded)
{
    QStringList notAddedDummy;
    if (!notAdded)
        notAdded = &notAddedDummy;

    QbsProjectNode *prjNode = parentQbsProjectNode(this);
    if (!prjNode || !prjNode->qbsProject().isValid()) {
        *notAdded += filePaths;
        return false;
    }

    QbsProductNode *prdNode = parentQbsProductNode(this);
    if (!prdNode || !prdNode->qbsProductData().isValid()) {
        *notAdded += filePaths;
        return false;
    }

    return prjNode->project()->addFilesToProduct(this, filePaths, prdNode->qbsProductData(),
                                                 m_qbsGroupData, notAdded);
}

bool QbsGroupNode::removeFiles(const QStringList &filePaths, QStringList *notRemoved)
{
    QStringList notRemovedDummy;
    if (!notRemoved)
        notRemoved = &notRemovedDummy;

    QbsProjectNode *prjNode = parentQbsProjectNode(this);
    if (!prjNode || !prjNode->qbsProject().isValid()) {
        *notRemoved += filePaths;
        return false;
    }

    QbsProductNode *prdNode = parentQbsProductNode(this);
    if (!prdNode || !prdNode->qbsProductData().isValid()) {
        *notRemoved += filePaths;
        return false;
    }

    return prjNode->project()->removeFilesFromProduct(this, filePaths, prdNode->qbsProductData(),
                                                      m_qbsGroupData, notRemoved);
}

bool QbsGroupNode::renameFile(const QString &filePath, const QString &newFilePath)
{
    QbsProjectNode * const prjNode = parentQbsProjectNode(this);
    if (!prjNode || !prjNode->qbsProject().isValid())
        return false;
    QbsProductNode * const prdNode = parentQbsProductNode(this);
    if (!prdNode || !prdNode->qbsProductData().isValid())
        return false;

    return prjNode->project()->renameFileInProduct(this, filePath, newFilePath,
                                                   prdNode->qbsProductData(), m_qbsGroupData);
}

void QbsGroupNode::updateQbsGroupData(const qbs::GroupData &grp, const QString &productPath,
                                      bool productIsEnabled)
{
    QTC_ASSERT(grp.isValid(), return);

    setEnabled(productIsEnabled && grp.isEnabled());

    m_productPath = productPath;
    m_qbsGroupData = grp;

    setAbsoluteFilePathAndLine(Utils::FileName::fromString(grp.location().filePath()), line());
    setDisplayName(grp.name());

    QbsFileNode *idx = 0;
    foreach (ProjectExplorer::FileNode *fn, fileNodes()) {
        idx = dynamic_cast<QbsFileNode *>(fn);
        if (idx)
            break;
    }
    QTC_ASSERT(idx, return);
    idx->setAbsoluteFilePathAndLine(Utils::FileName::fromString(grp.location().filePath()),
                        grp.location().line());

    setupFiles(this, grp, grp.allFilePaths(), productPath, false);
}

void QbsGroupNode::setupFiles(ProjectExplorer::FolderNode *root, const qbs::GroupData &group,
        const QStringList &files, const QString &productPath, bool generated)
{
    // Build up a tree of nodes:
    FileTreeNode tree;

    foreach (const QString &path, files) {
        QStringList pathSegments = path.split(QLatin1Char('/'), QString::SkipEmptyParts);

        FileTreeNode *root = &tree;
        while (!pathSegments.isEmpty()) {
            bool isFile = pathSegments.count() == 1;
            root = root->addPart(pathSegments.takeFirst(), isFile);
        }
    }

    FileTreeNode::reorder(&tree, productPath);
    FileTreeNode::simplify(&tree);

    QHash<QString, ProjectExplorer::FileType> fileTypeHash;
    foreach (const qbs::ArtifactData &sa, group.allSourceArtifacts())
        fileTypeHash[sa.filePath()] = fileType(sa);

    setupFolder(root, fileTypeHash, &tree, productPath, generated);
}

void QbsGroupNode::setupFolder(ProjectExplorer::FolderNode *root,
                               const QHash<QString, ProjectExplorer::FileType> &fileTypeHash,
                               const FileTreeNode *fileTree,
                               const QString &baseDir,
                               bool generated)
{
    // We only need to care about FileNodes and FolderNodes here. Everything else is
    // handled elsewhere.
    // QbsGroupNodes are managed by the QbsProductNode.
    // The buildsystem file is either managed by QbsProductNode or by updateQbsGroupData(...).

    foreach (FileTreeNode *c, fileTree->children) {
        Utils::FileName path = Utils::FileName::fromString(c->path());
        const ProjectExplorer::FileType newFileType =
                fileTypeHash.value(c->path(), ProjectExplorer::FileType::Unknown);
        const bool isQrcFile = newFileType == ProjectExplorer::FileType::Resource;

        // Handle files:
        if (c->isFile() && !isQrcFile) {
            ProjectExplorer::FileNode *fn = 0;
            foreach (ProjectExplorer::FileNode *f, root->fileNodes()) {
                // There can be one match only here!
                if (f->filePath() != path || f->fileType() != newFileType)
                    continue;
                fn = f;
                break;
            }
            if (!fn) {
                fn = new ProjectExplorer::FileNode(path, newFileType, generated);
                root->addNode(fn);
            }
            continue;
        } else {
            ProjectExplorer::FolderNode *fn = 0;
            foreach (ProjectExplorer::FolderNode *f, root->folderNodes()) {
                // There can be one match only here!
                if (f->filePath() != path)
                    continue;
                fn = f;
                break;
            }
            using ResourceEditor::ResourceTopLevelNode;
            if (!fn) {
                if (isQrcFile) {
                    fn = new ResourceTopLevelNode(Utils::FileName::fromString(c->path()), QString(), root);
                } else {
                    fn = new QbsFolderNode(Utils::FileName::fromString(c->path()),
                                           ProjectExplorer::NodeType::Folder,
                                           displayNameFromPath(c->path(), baseDir), false);
                }
                root->addNode(fn);
            } else {
                fn->setDisplayName(displayNameFromPath(c->path(), baseDir));
            }

            if (isQrcFile)
                static_cast<ResourceTopLevelNode *>(fn)->addInternalNodes();
            else
                setupFolder(fn, fileTypeHash, c, c->path(), generated);
        }
    }
}

ProjectExplorer::FileType QbsGroupNode::fileType(const qbs::ArtifactData &artifact)
{
    QTC_ASSERT(artifact.isValid(), return ProjectExplorer::FileType::Unknown);

    if (artifact.fileTags().contains(QLatin1String("c"))
            || artifact.fileTags().contains(QLatin1String("cpp"))
            || artifact.fileTags().contains(QLatin1String("objc"))
            || artifact.fileTags().contains(QLatin1String("objcpp"))) {
        return ProjectExplorer::FileType::Source;
    }
    if (artifact.fileTags().contains(QLatin1String("hpp")))
        return ProjectExplorer::FileType::Header;
    if (artifact.fileTags().contains(QLatin1String("qrc")))
        return ProjectExplorer::FileType::Resource;
    if (artifact.fileTags().contains(QLatin1String("ui")))
        return ProjectExplorer::FileType::Form;
    if (artifact.fileTags().contains(QLatin1String("scxml")))
        return ProjectExplorer::FileType::StateChart;
    return ProjectExplorer::FileType::Unknown;
}

// --------------------------------------------------------------------
// QbsProductNode:
// --------------------------------------------------------------------

QbsProductNode::QbsProductNode(const qbs::Project &project, const qbs::ProductData &prd) :
    QbsBaseProjectNode(Utils::FileName::fromString(prd.location().filePath())),
    m_generatedFilesNode(new QbsFolderNode(Utils::FileName::fromString(prd.buildDirectory()),
            ProjectExplorer::NodeType::Folder,
            QCoreApplication::translate("QbsProductNode", "Generated files"), true))
{
    if (m_productIcon.isNull())
        m_productIcon = generateIcon(QString::fromLatin1(Constants::QBS_PRODUCT_OVERLAY_ICON));

    setIcon(m_productIcon);

    addNode(m_generatedFilesNode);
    addNode(new QbsFileNode(Utils::FileName::fromString(prd.location().filePath()),
                            ProjectExplorer::FileType::Project, false,
                            prd.location().line()));

    setQbsProductData(project, prd);
}

bool QbsProductNode::showInSimpleTree() const
{
    return true;
}

QList<ProjectExplorer::ProjectAction> QbsProductNode::supportedActions(ProjectExplorer::Node *node) const
{
    return supportedNodeActions(node, true);
}

bool QbsProductNode::addFiles(const QStringList &filePaths, QStringList *notAdded)
{
    QStringList notAddedDummy;
    if (!notAdded)
        notAdded = &notAddedDummy;

    QbsProjectNode *prjNode = parentQbsProjectNode(this);
    if (!prjNode || !prjNode->qbsProject().isValid()) {
        *notAdded += filePaths;
        return false;
    }

    qbs::GroupData grp = findMainQbsGroup(m_qbsProductData);
    if (grp.isValid()) {
        return prjNode->project()->addFilesToProduct(this, filePaths, m_qbsProductData, grp,
                                                     notAdded);
    }

    QTC_ASSERT(false, return false);
}

bool QbsProductNode::removeFiles(const QStringList &filePaths, QStringList *notRemoved)
{
    QStringList notRemovedDummy;
    if (!notRemoved)
        notRemoved = &notRemovedDummy;

    QbsProjectNode *prjNode = parentQbsProjectNode(this);
    if (!prjNode || !prjNode->qbsProject().isValid()) {
        *notRemoved += filePaths;
        return false;
    }

    qbs::GroupData grp = findMainQbsGroup(m_qbsProductData);
    if (grp.isValid()) {
        return prjNode->project()->removeFilesFromProduct(this, filePaths, m_qbsProductData, grp,
                                                          notRemoved);
    }

    QTC_ASSERT(false, return false);
}

bool QbsProductNode::renameFile(const QString &filePath, const QString &newFilePath)
{
    QbsProjectNode * const prjNode = parentQbsProjectNode(this);
    if (!prjNode || !prjNode->qbsProject().isValid())
        return false;
    const qbs::GroupData grp = findMainQbsGroup(m_qbsProductData);
    QTC_ASSERT(grp.isValid(), return false);
    return prjNode->project()->renameFileInProduct(this, filePath, newFilePath, m_qbsProductData,
                                                   grp);
}

void QbsProductNode::setQbsProductData(const qbs::Project &project, const qbs::ProductData prd)
{
    if (m_qbsProductData == prd)
        return;

    setEnabled(prd.isEnabled());

    setDisplayName(QbsProject::productDisplayName(project, prd));
    setAbsoluteFilePathAndLine(Utils::FileName::fromString(prd.location().filePath()), line());
    const QString &productPath = QFileInfo(prd.location().filePath()).absolutePath();

    // Find the QbsFileNode we added earlier:
    QbsFileNode *idx = 0;
    foreach (ProjectExplorer::FileNode *fn, fileNodes()) {
        idx = dynamic_cast<QbsFileNode *>(fn);
        if (idx)
            break;
    }
    QTC_ASSERT(idx, return);
    idx->setAbsoluteFilePathAndLine(Utils::FileName::fromString(prd.location().filePath()),
                        prd.location().line());

    foreach (const qbs::GroupData &grp, prd.groups()) {
        if (grp.name() == prd.name() && grp.location() == prd.location()) {
            // Set implicit product group right onto this node:
            QbsGroupNode::setupFiles(this, grp, grp.allFilePaths(), productPath, false);
            continue;
        }
        addNode(new QbsGroupNode(grp, productPath));
    }

    if (prd.isEnabled()) {
        const QStringList generatedFiles
                = Utils::transform(prd.generatedArtifacts(), &qbs::ArtifactData::filePath);
        QbsGroupNode::setupFiles(m_generatedFilesNode, qbs::GroupData(), generatedFiles,
                                 prd.buildDirectory(), true);
    }

    m_qbsProductData = prd;
}

QList<ProjectExplorer::RunConfiguration *> QbsProductNode::runConfigurations() const
{
    QList<ProjectExplorer::RunConfiguration *> result;
    auto pn = dynamic_cast<const QbsProjectNode *>(managingProject());
    if (!isEnabled() || !pn || m_qbsProductData.targetExecutable().isEmpty())
        return result;

    foreach (ProjectExplorer::RunConfiguration *rc, pn->project()->activeTarget()->runConfigurations()) {
        QbsRunConfiguration *qbsRc = qobject_cast<QbsRunConfiguration *>(rc);
        if (!qbsRc)
            continue;
        if (qbsRc->buildSystemTarget() == QbsProject::uniqueProductName(qbsProductData()))
            result << qbsRc;
    }

    return result;
}

QbsGroupNode *QbsProductNode::findGroupNode(const QString &name)
{
    for (ProjectExplorer::Node *n : nodes()) {
        if (QbsGroupNode *qn = dynamic_cast<QbsGroupNode *>(n))
            if (qn->qbsGroupData().name() == name)
                return qn;
    }
    return 0;
}

// --------------------------------------------------------------------
// QbsProjectNode:
// --------------------------------------------------------------------

QbsProjectNode::QbsProjectNode(const Utils::FileName &path) :
    QbsBaseProjectNode(path)
{
    ctor();
}

QbsProjectNode::~QbsProjectNode()
{
    // do not delete m_project
}

void QbsProjectNode::update(const qbs::Project &qbsProject, const qbs::ProjectData &prjData)
{
    foreach (const qbs::ProjectData &subData, prjData.subProjects()) {
        auto subProject =
                new QbsProjectNode(Utils::FileName::fromString(subData.location().filePath()));
        subProject->update(qbsProject, subData);
        addNode(subProject);
    }

    foreach (const qbs::ProductData &prd, prjData.products())
        addNode(new QbsProductNode(qbsProject, prd));

    if (!prjData.name().isEmpty())
        setDisplayName(prjData.name());
    else
        setDisplayName(project()->displayName());

    m_projectData = prjData;
}

QbsProject *QbsProjectNode::project() const
{
    return static_cast<QbsProjectNode *>(parentFolderNode())->project();
}

const qbs::Project QbsProjectNode::qbsProject() const
{
    return project()->qbsProject();
}

bool QbsProjectNode::showInSimpleTree() const
{
    return true;
}

void QbsProjectNode::ctor()
{
    if (m_projectIcon.isNull())
        m_projectIcon = generateIcon(QString::fromLatin1(ProjectExplorer::Constants::FILEOVERLAY_QT));

    setIcon(m_projectIcon);
    addNode(new ProjectExplorer::FileNode(filePath(), ProjectExplorer::FileType::Project, false));
}

QbsProductNode *QbsProjectNode::findProductNode(const QString &uniqueName)
{
    for (ProjectExplorer::Node *n : nodes()) {
        if (QbsProductNode *qn = dynamic_cast<QbsProductNode *>(n))
            if (QbsProject::uniqueProductName(qn->qbsProductData()) == uniqueName)
                return qn;
    }
    return 0;
}

QbsProjectNode *QbsProjectNode::findProjectNode(const QString &name)
{
    for (ProjectExplorer::Node *n : nodes()) {
        if (QbsProjectNode *qn = dynamic_cast<QbsProjectNode *>(n))
            if (qn->qbsProjectData().name() == name)
                return qn;
    }
    return 0;
}

// --------------------------------------------------------------------
// QbsRootProjectNode:
// --------------------------------------------------------------------

QbsRootProjectNode::QbsRootProjectNode(QbsProject *project) :
    QbsProjectNode(project->projectFilePath()),
    m_project(project),
    m_buildSystemFiles(new ProjectExplorer::FolderNode(project->projectDirectory(),
                                                       ProjectExplorer::NodeType::Folder,
                                                       QCoreApplication::translate("QbsRootProjectNode", "Qbs files")))
{
    addNode(m_buildSystemFiles);
}

void QbsRootProjectNode::update()
{
    QStringList buildSystemFiles = unreferencedBuildSystemFiles(m_project->qbsProject());

    QStringList projectBuildSystemFiles;
    Utils::FileName base = m_project->projectDirectory();
    foreach (const QString &f, buildSystemFiles) {
        if (Utils::FileName::fromString(f).isChildOf(base))
                projectBuildSystemFiles.append(f);
    }
    QbsGroupNode::setupFiles(m_buildSystemFiles, qbs::GroupData(), projectBuildSystemFiles,
                             base.toString(), false);

    update(m_project->qbsProject(), m_project->qbsProjectData());
}

static QSet<QString> referencedBuildSystemFiles(const qbs::ProjectData &data)
{
    QSet<QString> result;
    result.insert(data.location().filePath());
    foreach (const qbs::ProjectData &subProject, data.subProjects())
        result.unite(referencedBuildSystemFiles(subProject));
    foreach (const qbs::ProductData &product, data.products()) {
        result.insert(product.location().filePath());
        foreach (const qbs::GroupData &group, product.groups())
            result.insert(group.location().filePath());
    }

    return result;
}

QStringList QbsRootProjectNode::unreferencedBuildSystemFiles(const qbs::Project &p) const
{
    return p.buildSystemFiles().subtract(referencedBuildSystemFiles(p.projectData())).toList();
}

} // namespace Internal
} // namespace QbsProjectManager
