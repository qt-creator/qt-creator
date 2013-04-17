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

#include <coreplugin/fileiconprovider.h>
#include <qtsupport/qtsupportconstants.h>
#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>

#include <qbs.h>

#include <QDir>
#include <QStyle>

// ----------------------------------------------------------------------
// Helpers:
// ----------------------------------------------------------------------

namespace QbsProjectManager {
namespace Internal {

QIcon generateIcon()
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

QIcon QbsProjectNode::m_projectIcon = generateIcon();
QIcon QbsProductNode::m_productIcon = generateIcon();
QIcon QbsGroupNode::m_groupIcon = generateIcon();

class FileTreeNode {
public:
    FileTreeNode(const QString &n, FileTreeNode *p = 0) :
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

    // Moves the node pointing to basedir to the root of the tree and deletes any now empty nodes.
    static void reorder(FileTreeNode *node, const QString &basedir, FileTreeNode *root)
    {
        if (node != root && node->path() == basedir) {
            // move node to root:
            FileTreeNode *parent = node->parent;
            if (parent)
                parent->children.removeOne(node);
            root->children.append(node);
            node->parent = root;
            if (basedir.startsWith(QLatin1Char('/')))
                node->name = basedir.mid(1);
            else
                node->name = basedir;

            // clean up now-empty nodes:
            while (parent) {
                if (parent->children.count() == 0) {
                    FileTreeNode *current = parent;
                    parent = current->parent;
                    parent->children.removeOne(current);
                    current->parent = 0;
                    delete current;
                } else {
                    break;
                }
            }
            return;
        }
        foreach (FileTreeNode *n, node->children)
            reorder(n, basedir, root);
    }

    static void simplify(FileTreeNode *node)
    {
        foreach (FileTreeNode *c, node->children)
            simplify(c);

        if (!node->parent)
            return;

        if (node->children.count() == 1) {
            FileTreeNode *child = node->children.at(0);
            if (child->isFile())
                return;

            if (!Utils::HostOsInfo::isWindowsHost() || !node->name.isEmpty())
                node->name = node->name + QLatin1Char('/') + child->name;
            else
                node->name = child->name;
            node->children = child->children;

            foreach (FileTreeNode *tmpChild, node->children)
                tmpChild->parent = node;

            child->children.clear();
            child->parent = 0;
            delete child;
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
    m_group(0)
{
    setIcon(m_groupIcon);
    setGroup(grp, productPath);
}

bool QbsGroupNode::isEnabled() const
{
    return static_cast<QbsProductNode *>(parentFolderNode())->isEnabled() && group()->isEnabled();
}

void QbsGroupNode::setGroup(const qbs::GroupData *group, const QString &productPath)
{
    if (group == m_group && productPath == m_productPath)
        return;

    m_productPath = productPath;

    // Set Product file node used to jump to the product
    setPath(group->location().fileName);
    setDisplayName(group->name());

    // set up file node...
    QbsFileNode *indexFile = 0;
    if (!m_group) {
        indexFile = new QbsFileNode(group->location().fileName,
                                    ProjectExplorer::ProjectFileType, false,
                                    group->location().line);
        addFileNodes(QList<ProjectExplorer::FileNode *>() << indexFile, this);
    } else {
        indexFile = static_cast<QbsFileNode *>(fileNodes().first());
        indexFile->setPath(group->location().fileName);
        indexFile->setLine(group->location().line);
        indexFile->emitNodeUpdated();
    }

    m_group = group;

    setGroup(this, group, productPath, QList<ProjectExplorer::Node *>() << indexFile);
    emitNodeUpdated();
}

void QbsGroupNode::setGroup(QbsBaseProjectNode *root, const qbs::GroupData *group,
                            const QString &productPath, QList<ProjectExplorer::Node *> keepers)
{
    // Build up a tree of nodes:
    FileTreeNode *tree = new FileTreeNode(QString());

    foreach (const QString &path, group->allFilePaths()) {
        QStringList pathSegments = path.split(QLatin1Char('/'), QString::SkipEmptyParts);

        FileTreeNode *root = tree;
        while (!pathSegments.isEmpty())
            root = root->addPart(pathSegments.takeFirst());
    }

    FileTreeNode::reorder(tree, productPath, tree);
    FileTreeNode::simplify(tree);
    setupFolders(root, root, tree, productPath, keepers);
    delete tree;
}

void QbsGroupNode::setupFolders(QbsBaseProjectNode *topLevel, ProjectExplorer::FolderNode *root,
                                FileTreeNode *node, const QString &baseDirPath,
                                QList<ProjectExplorer::Node *> keepers)
{
    QList<ProjectExplorer::FileNode *> filesToRemove;
    foreach (ProjectExplorer::FileNode *fn, root->fileNodes()) {
        ProjectExplorer::Node *n = static_cast<ProjectExplorer::Node *>(fn);
        if (!keepers.contains(n))
            filesToRemove.append(fn);
    }

    QList<ProjectExplorer::FileNode *> filesToAdd;

    QList<ProjectExplorer::Node *> foldersToKeep = keepers;
    QList<ProjectExplorer::FolderNode *> foldersToRemove;
    foreach (ProjectExplorer::FolderNode *fn, root->subFolderNodes()) {
        ProjectExplorer::Node *n = static_cast<ProjectExplorer::Node *>(fn);
        if (!keepers.contains(n))
            foldersToRemove.append(fn);
    }

    // insert subfolders
    foreach (FileTreeNode *c, node->children) {
        QString path = c->path();
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
        }

        ProjectExplorer::FolderNode *fn = root->findSubFolder(path);
        if (path == baseDirPath) {
            setupFolders(topLevel, root, c, c->path(), foldersToKeep);
            continue;
        }

        if (path.startsWith(baseDirPath + QLatin1Char('/')))
            path = path.mid(baseDirPath.length() + 1); // remove common prefix

        if (fn) {
            fn->emitNodeUpdated(); // enabled might have changed
            foldersToRemove.removeOne(fn);
        } else {
            fn = new ProjectExplorer::FolderNode(path);
            topLevel->addFolderNodes(QList<ProjectExplorer::FolderNode *>() << fn, root);
        }
        foldersToKeep.append(fn);
        setupFolders(topLevel, fn, c, c->path());
    }
    topLevel->removeFileNodes(filesToRemove, root);
    topLevel->removeFolderNodes(foldersToRemove, root);
    topLevel->addFileNodes(filesToAdd, root);
}

// --------------------------------------------------------------------
// QbsProductNode:
// --------------------------------------------------------------------

QbsProductNode::QbsProductNode(const qbs::ProductData *prd) :
    QbsBaseProjectNode(prd->location().fileName),
    m_product(0)
{
    setIcon(m_productIcon);
    setProduct(prd);
}

bool QbsProductNode::isEnabled() const
{
    return product()->isEnabled();
}

void QbsProductNode::setProduct(const qbs::ProductData *prd)
{
    if (m_product == prd)
        return;

    setDisplayName(prd->name());
    setPath(prd->location().fileName);
    const QString &productPath = QFileInfo(prd->location().fileName).absolutePath();

    // Set Product file node used to jump to the product
    QList<ProjectExplorer::FileNode *> files = fileNodes();
    QList<ProjectExplorer::Node *> toKeep;
    if (files.isEmpty()) {
        QbsFileNode *idx = new QbsFileNode(prd->location().fileName,
                                                         ProjectExplorer::ProjectFileType, false,
                                                         prd->location().line);
        addFileNodes(QList<ProjectExplorer::FileNode *>() << idx, this);
        toKeep.append(idx);
    } else {
        QbsFileNode *idx = static_cast<QbsFileNode *>(files.at(0));
        idx->setPath(prd->location().fileName);
        idx->setLine(prd->location().line);
        toKeep.append(idx);
    }

    QList<ProjectExplorer::ProjectNode *> toAdd;
    QList<ProjectExplorer::ProjectNode *> toRemove = subProjectNodes();

    foreach (const qbs::GroupData &grp, prd->groups()) {
        if (grp.name() == prd->name() && grp.location() == prd->location()) {
            // Set implicit product group right onto this node:
            QbsGroupNode::setGroup(this, &grp, productPath, toKeep);
            continue;
        }
        QbsGroupNode *qn = findGroupNode(grp.name());
        if (qn) {
            toRemove.removeAll(qn);
            toKeep.append(qn);
            qn->setGroup(&grp, productPath);
        } else {
            qn = new QbsGroupNode(&grp, productPath);
            toAdd.append(qn);
            toKeep.append(qn);
        }
    }

    addProjectNodes(toAdd);
    removeProjectNodes(toRemove);

    m_product = prd;
    emitNodeUpdated();
}

QbsGroupNode *QbsProductNode::findGroupNode(const QString &name)
{
    foreach (ProjectExplorer::ProjectNode *n, subProjectNodes()) {
        QbsGroupNode *qn = static_cast<QbsGroupNode *>(n);
        if (qn->group()->name() == name)
            return qn;
    }
    return 0;
}

// --------------------------------------------------------------------
// QbsProjectNode:
// --------------------------------------------------------------------

QbsProjectNode::QbsProjectNode(const QString &projectFile) :
    QbsBaseProjectNode(projectFile),
    m_project(0), m_projectData(0)
{
    setIcon(m_projectIcon);
    addFileNodes(QList<ProjectExplorer::FileNode *>()
                 << new ProjectExplorer::FileNode(projectFile, ProjectExplorer::ProjectFileType, false), this);
}

QbsProjectNode::~QbsProjectNode()
{
    delete m_projectData;
    delete m_project;
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
                qn->setProduct(&prd);
                toRemove.removeOne(qn);
            }
        }
    }

    delete m_projectData;
    m_projectData = newData;

    delete m_project;
    m_project = prj;

    removeProjectNodes(toRemove);
    addProjectNodes(toAdd);
}

const qbs::Project *QbsProjectNode::project() const
{
    return m_project;
}

const qbs::ProjectData *QbsProjectNode::projectData() const
{
    return m_projectData;
}

QbsProductNode *QbsProjectNode::findProductNode(const QString &name)
{
    foreach (ProjectExplorer::ProjectNode *n, subProjectNodes()) {
        QbsProductNode *qn = static_cast<QbsProductNode *>(n);
        if (qn->product()->name() == name)
            return qn;
    }
    return 0;
}

} // namespace Internal
} // namespace QbsProjectManager
