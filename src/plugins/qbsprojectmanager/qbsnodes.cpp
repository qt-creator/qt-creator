/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "qbsnodes.h"

#include "qbsproject.h"
#include "qbsrunconfiguration.h"

#include <coreplugin/fileiconprovider.h>
#include <coreplugin/idocument.h>
#include <projectexplorer/target.h>
#include <qtsupport/qtsupportconstants.h>
#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>

#include <qbs.h>

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

static QIcon generateIcon()
{
    const QSize desiredSize = QSize(16, 16);
    const QIcon projectBaseIcon(QString::fromLatin1(QtSupport::Constants::ICON_QT_PROJECT));
    const QPixmap projectPixmap = Core::FileIconProvider::overlayIcon(QStyle::SP_DirIcon,
                                                                      projectBaseIcon,
                                                                      desiredSize);

    QIcon result;
    result.addPixmap(projectPixmap);

    return result;
}

namespace QbsProjectManager {
namespace Internal {

QIcon QbsProjectNode::m_projectIcon = generateIcon();
QIcon QbsProductNode::m_productIcon = generateIcon();
QIcon QbsGroupNode::m_groupIcon = generateIcon();

class FileTreeNode {
public:
    explicit FileTreeNode(const QString &n = QString(), FileTreeNode *p = 0) :
        parent(p), name(n)
    {
        if (p)
            p->children.append(this);
    }

    ~FileTreeNode()
    {
        qDeleteAll(children);
    }

    FileTreeNode *addPart(const QString &n)
    {
        foreach (FileTreeNode *c, children) {
            if (c->name == n)
                return c;
        }
        return new FileTreeNode(n, this);
    }

    bool isFile() { return children.isEmpty(); }

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

        if (node->path() == basedir) {
            // Find root node:
            FileTreeNode *root = node;
            while (root->parent)
                root = root->parent;

            foreach (FileTreeNode *c, node->children) {
                // Update children names by prepending basedir
                c->name = basedir + QLatin1Char('/') + c->name;
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

        if (node->children.count() == 0 && !node->isFile()) {
            // Clean up empty folder nodes:
            node->parent->children.removeOne(node);
            node->parent = 0;
            delete node;
        } else if (node->children.count() == 1 && !node->children.at(0)->isFile()) {
            // Compact folder nodes with one child only:
            moveChildrenUp(node);
        }
    }

    QString path()
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
};

// ----------------------------------------------------------------------
// QbsFileNode:
// ----------------------------------------------------------------------

QbsFileNode::QbsFileNode(const QString &filePath, const ProjectExplorer::FileType fileType,
                         bool generated, int line) :
    ProjectExplorer::FileNode(filePath, fileType, generated),
    m_line(line)
{ }

void QbsFileNode::setLine(int l)
{
    m_line = l;
}

// ---------------------------------------------------------------------------
// QbsBaseProjectNode:
// ---------------------------------------------------------------------------

QbsBaseProjectNode::QbsBaseProjectNode(const QString &path) :
    ProjectExplorer::ProjectNode(path)
{ }

bool QbsBaseProjectNode::hasBuildTargets() const
{
    foreach (ProjectNode *n, subProjectNodes())
        if (n->hasBuildTargets())
            return true;
    return false;
}

QList<ProjectExplorer::ProjectNode::ProjectAction> QbsBaseProjectNode::supportedActions(ProjectExplorer::Node *node) const
{
    Q_UNUSED(node);
    return QList<ProjectExplorer::ProjectNode::ProjectAction>();
}

bool QbsBaseProjectNode::canAddSubProject(const QString &proFilePath) const
{
    Q_UNUSED(proFilePath);
    return false;
}

bool QbsBaseProjectNode::addSubProjects(const QStringList &proFilePaths)
{
    Q_UNUSED(proFilePaths);
    return false;
}

bool QbsBaseProjectNode::removeSubProjects(const QStringList &proFilePaths)
{
    Q_UNUSED(proFilePaths);
    return false;
}

bool QbsBaseProjectNode::addFiles(const ProjectExplorer::FileType fileType, const QStringList &filePaths, QStringList *notAdded)
{
    Q_UNUSED(fileType);
    Q_UNUSED(filePaths);
    Q_UNUSED(notAdded);
    return false;
}

bool QbsBaseProjectNode::removeFiles(const ProjectExplorer::FileType fileType, const QStringList &filePaths, QStringList *notRemoved)
{
    Q_UNUSED(fileType);
    Q_UNUSED(filePaths);
    Q_UNUSED(notRemoved);
    return false;
}

bool QbsBaseProjectNode::deleteFiles(const ProjectExplorer::FileType fileType, const QStringList &filePaths)
{
    Q_UNUSED(fileType);
    Q_UNUSED(filePaths);
    return false;
}

bool QbsBaseProjectNode::renameFile(const ProjectExplorer::FileType fileType, const QString &filePath, const QString &newFilePath)
{
    Q_UNUSED(fileType);
    Q_UNUSED(filePath);
    Q_UNUSED(newFilePath);
    return false;
}

QList<ProjectExplorer::RunConfiguration *> QbsBaseProjectNode::runConfigurationsFor(ProjectExplorer::Node *node)
{
    Q_UNUSED(node);
    return QList<ProjectExplorer::RunConfiguration *>();
}

// --------------------------------------------------------------------
// QbsGroupNode:
// --------------------------------------------------------------------

QbsGroupNode::QbsGroupNode(const qbs::GroupData *grp, const QString &productPath) :
    QbsBaseProjectNode(QString()),
    m_qbsGroupData(0)
{
    setIcon(m_groupIcon);

    QbsFileNode *idx = new QbsFileNode(grp->location().fileName(),
                                       ProjectExplorer::ProjectFileType, false,
                                       grp->location().line());
    addFileNodes(QList<ProjectExplorer::FileNode *>() << idx, this);

    updateQbsGroupData(grp, productPath);
}

bool QbsGroupNode::isEnabled() const
{
    return static_cast<QbsProductNode *>(parentFolderNode())->isEnabled() && qbsGroupData()->isEnabled();
}

void QbsGroupNode::updateQbsGroupData(const qbs::GroupData *grp, const QString &productPath)
{
    if (grp == m_qbsGroupData && productPath == m_productPath)
        return;

    m_productPath = productPath;
    m_qbsGroupData = grp;

    setPath(grp->location().fileName());
    setDisplayName(grp->name());

    QbsFileNode *idx = 0;
    foreach (ProjectExplorer::FileNode *fn, fileNodes()) {
        idx = qobject_cast<QbsFileNode *>(fn);
        if (idx)
            break;
    }

    // idx not found, which should never happen!
    Q_ASSERT(idx);
    idx->setPath(grp->location().fileName());
    idx->setLine(grp->location().line());

    setupFiles(this, grp->allFilePaths(), m_productPath);
    emitNodeUpdated();
}

void QbsGroupNode::setupFiles(QbsBaseProjectNode *root, const QStringList &files,
                              const QString &productPath)
{
    // Build up a tree of nodes:
    FileTreeNode tree;

    foreach (const QString &path, files) {
        QStringList pathSegments = path.split(QLatin1Char('/'), QString::SkipEmptyParts);

        FileTreeNode *root = &tree;
        while (!pathSegments.isEmpty())
            root = root->addPart(pathSegments.takeFirst());
    }

    FileTreeNode::reorder(&tree, productPath);
    FileTreeNode::simplify(&tree);

    setupFolder(root, &tree, productPath);
}

void QbsGroupNode::setupFolder(ProjectExplorer::FolderNode *root,
                               const FileTreeNode *fileTree, const QString &baseDir)
{
    // We only need to care about FileNodes and FolderNodes here. Everything else is
    // handled elsewhere.
    // QbsGroupNodes are managed by the QbsProductNode.
    // The buildsystem file is either managed by QbsProductNode or by updateQbsGroupData(...).

    QList<ProjectExplorer::FileNode *> filesToRemove;
    foreach (ProjectExplorer::FileNode *fn, root->fileNodes()) {
        if (!qobject_cast<QbsFileNode *>(fn))
            filesToRemove << fn;
    }
    QList<ProjectExplorer::FileNode *> filesToAdd;

    QList<ProjectExplorer::FolderNode *> foldersToRemove;
    foreach (ProjectExplorer::FolderNode *fn, root->subFolderNodes()) {
        if (fn->nodeType() == ProjectExplorer::ProjectNodeType)
            continue; // Skip ProjectNodes mixed into the folders...
        foldersToRemove.append(fn);
    }

    foreach (FileTreeNode *c, fileTree->children) {
        QString path = c->path();

        // Handle files:
        if (c->isFile()) {
            ProjectExplorer::FileNode *fn = root->findFile(path);
            if (fn) {
                fn->emitNodeUpdated(); // enabled might have changed
                filesToRemove.removeOne(fn);
            } else {
                fn = new ProjectExplorer::FileNode(path, ProjectExplorer::UnknownFileType, false);
                filesToAdd.append(fn);
            }
            continue;
        } else {
            FolderNode *fn = root->findSubFolder(c->path());
            if (!fn) {
                fn = new FolderNode(c->path());
                root->projectNode()->addFolderNodes(QList<FolderNode *>() << fn, root);
            } else {
                foldersToRemove.removeOne(fn);
            }
            fn->setDisplayName(displayNameFromPath(c->path(), baseDir));

            setupFolder(fn, c, c->path());
        }
    }
    root->projectNode()->removeFileNodes(filesToRemove, root);
    root->projectNode()->removeFolderNodes(foldersToRemove, root);
    root->projectNode()->addFileNodes(filesToAdd, root);
}

// --------------------------------------------------------------------
// QbsProductNode:
// --------------------------------------------------------------------

QbsProductNode::QbsProductNode(const qbs::ProductData *prd) :
    QbsBaseProjectNode(prd->location().fileName()),
    m_qbsProductData(0)
{
    setIcon(m_productIcon);
    setQbsProductData(prd);
}

bool QbsProductNode::isEnabled() const
{
    return qbsProductData()->isEnabled();
}

void QbsProductNode::setQbsProductData(const qbs::ProductData *prd)
{
    if (m_qbsProductData == prd)
        return;

    setDisplayName(prd->name());
    setPath(prd->location().fileName());
    const QString &productPath = QFileInfo(prd->location().fileName()).absolutePath();

    // Set Product file node used to jump to the product
    QList<ProjectExplorer::FileNode *> files = fileNodes();
    QbsFileNode *idx = 0;
    if (files.isEmpty()) {
        idx = new QbsFileNode(prd->location().fileName(),
                              ProjectExplorer::ProjectFileType, false,
                              prd->location().line());
        addFileNodes(QList<ProjectExplorer::FileNode *>() << idx, this);
    } else {
        // Find the QbsFileNode we added earlier:
        foreach (ProjectExplorer::FileNode *fn, files) {
            idx = qobject_cast<QbsFileNode *>(fn);
            if (idx) {
                idx->setPath(prd->location().fileName());
                idx->setLine(prd->location().line());
                break;
            }
        }
    }

    QList<ProjectExplorer::ProjectNode *> toAdd;
    QList<ProjectExplorer::ProjectNode *> toRemove = subProjectNodes();

    foreach (const qbs::GroupData &grp, prd->groups()) {
        if (grp.name() == prd->name() && grp.location() == prd->location()) {
            // Set implicit product group right onto this node:
            QbsGroupNode::setupFiles(this, grp.allFilePaths(), productPath);
            continue;
        }
        QbsGroupNode *gn = findGroupNode(grp.name());
        if (gn) {
            toRemove.removeOne(gn);
            gn->updateQbsGroupData(&grp, productPath);
        } else {
            gn = new QbsGroupNode(&grp, productPath);
            toAdd.append(gn);
        }
    }

    addProjectNodes(toAdd);
    removeProjectNodes(toRemove);

    m_qbsProductData = prd;
    emitNodeUpdated();
}

QList<ProjectExplorer::RunConfiguration *> QbsProductNode::runConfigurationsFor(ProjectExplorer::Node *node)
{
    Q_UNUSED(node);
    QList<ProjectExplorer::RunConfiguration *> result;
    QbsProjectNode *pn = qobject_cast<QbsProjectNode *>(projectNode());
    if (!isEnabled() || !pn || pn->qbsProject()->targetExecutable(*m_qbsProductData,
                                                                  qbs::InstallOptions()).isEmpty()) {
        return result;
    }

    foreach (ProjectExplorer::RunConfiguration *rc, pn->project()->activeTarget()->runConfigurations()) {
        QbsRunConfiguration *qbsRc = qobject_cast<QbsRunConfiguration *>(rc);
        if (!qbsRc)
            continue;
        if (qbsRc->qbsProduct() == qbsProductData()->name())
            result << qbsRc;
    }

    return result;
}

QbsGroupNode *QbsProductNode::findGroupNode(const QString &name)
{
    foreach (ProjectExplorer::ProjectNode *n, subProjectNodes()) {
        QbsGroupNode *qn = static_cast<QbsGroupNode *>(n);
        if (qn->qbsGroupData()->name() == name)
            return qn;
    }
    return 0;
}

// --------------------------------------------------------------------
// QbsProjectNode:
// --------------------------------------------------------------------

QbsProjectNode::QbsProjectNode(QbsProject *project) :
    QbsBaseProjectNode(project->document()->fileName()),
    m_project(project), m_qbsProject(0), m_qbsProjectData(0)
{
    Q_ASSERT(project);
    setIcon(m_projectIcon);
    addFileNodes(QList<ProjectExplorer::FileNode *>()
                 << new ProjectExplorer::FileNode(path(), ProjectExplorer::ProjectFileType, false), this);
}

QbsProjectNode::~QbsProjectNode()
{
    // do not delete m_project
    delete m_qbsProjectData;
    delete m_qbsProject;
}

void QbsProjectNode::update(const qbs::Project *prj)
{
    QList<ProjectExplorer::ProjectNode *> toAdd;
    QList<ProjectExplorer::ProjectNode *> toRemove = subProjectNodes();

    qbs::ProjectData *newData = 0;

    if (prj) {
        newData = new qbs::ProjectData(prj->projectData());
        foreach (const qbs::ProductData &prd, newData->products()) {
            QbsProductNode *qn = findProductNode(prd.name());
            if (!qn) {
                toAdd << new QbsProductNode(&prd);
            } else {
                qn->setQbsProductData(&prd);
                toRemove.removeOne(qn);
            }
        }
    }

    delete m_qbsProjectData;
    m_qbsProjectData = newData;

    delete m_qbsProject;
    m_qbsProject = prj;

    removeProjectNodes(toRemove);
    addProjectNodes(toAdd);
}

QbsProject *QbsProjectNode::project() const
{
    return m_project;
}

const qbs::Project *QbsProjectNode::qbsProject() const
{
    return m_qbsProject;
}

const qbs::ProjectData *QbsProjectNode::qbsProjectData() const
{
    return m_qbsProjectData;
}

QbsProductNode *QbsProjectNode::findProductNode(const QString &name)
{
    foreach (ProjectExplorer::ProjectNode *n, subProjectNodes()) {
        QbsProductNode *qn = static_cast<QbsProductNode *>(n);
        if (qn->qbsProductData()->name() == name)
            return qn;
    }
    return 0;
}

} // namespace Internal
} // namespace QbsProjectManager
