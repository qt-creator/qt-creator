/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "resourcenode.h"
#include "resourceeditorconstants.h"
#include "qrceditor/resourcefile_p.h"

#include <utils/fileutils.h>

#include <coreplugin/documentmanager.h>
#include <coreplugin/fileiconprovider.h>

#include <qmljstools/qmljstoolsconstants.h>

#include <utils/mimetypes/mimedatabase.h>

#include <QCoreApplication>
#include <QDir>
#include <QDebug>

using namespace ResourceEditor;
using namespace ResourceEditor::Internal;

static bool priority(const QStringList &files)
{
    if (files.isEmpty())
        return false;
    Utils::MimeDatabase mdb;
    QString type = mdb.mimeTypeForFile(files.at(0)).name();
    if (type.startsWith(QLatin1String("image/"))
            || type == QLatin1String(QmlJSTools::Constants::QML_MIMETYPE)
            || type == QLatin1String(QmlJSTools::Constants::JS_MIMETYPE))
        return true;
    return false;
}

static bool addFilesToResource(const Utils::FileName &resourceFile,
                               const QStringList &filePaths,
                               QStringList *notAdded,
                               const QString &prefix,
                               const QString &lang)
{
    if (notAdded)
        *notAdded = filePaths;

    ResourceFile file(resourceFile.toString());
    if (file.load() != Core::IDocument::OpenResult::Success)
        return false;

    int index = file.indexOfPrefix(prefix, lang);
    if (index == -1)
        index = file.addPrefix(prefix, lang);

    if (notAdded)
        notAdded->clear();
    foreach (const QString &path, filePaths) {
        if (file.contains(index, path)) {
            if (notAdded)
                *notAdded << path;
        } else {
            file.addFile(index, path);
        }
    }

    Core::DocumentManager::expectFileChange(resourceFile.toString());
    file.save();
    Core::DocumentManager::unexpectFileChange(resourceFile.toString());

    return true;
}

static bool sortByPrefixAndLang(ProjectExplorer::FolderNode *a, ProjectExplorer::FolderNode *b)
{
    ResourceFolderNode *aa = static_cast<ResourceFolderNode *>(a);
    ResourceFolderNode *bb = static_cast<ResourceFolderNode *>(b);

    if (aa->prefix() < bb->prefix())
        return true;
    if (bb->prefix() < aa->prefix())
        return false;
    return aa->lang() < bb->lang();
}

static bool sortNodesByPath(ProjectExplorer::Node *a, ProjectExplorer::Node *b)
{
    return a->path() < b->path();
}

ResourceTopLevelNode::ResourceTopLevelNode(const Utils::FileName &filePath, FolderNode *parent)
    : ProjectExplorer::FolderNode(filePath)
{
    setIcon(Core::FileIconProvider::icon(filePath.toString()));
    m_document = new ResourceFileWatcher(this);
    Core::DocumentManager::addDocument(m_document);

    Utils::FileName base = parent->path();
    if (filePath.isChildOf(base))
        setDisplayName(filePath.relativeChildPath(base).toString());
    else
        setDisplayName(filePath.toString());
}

ResourceTopLevelNode::~ResourceTopLevelNode()
{
    Core::DocumentManager::removeDocument(m_document);
    delete m_document;
}

void ResourceTopLevelNode::update()
{
    QList<ProjectExplorer::FolderNode *> newFolderList;
    QMap<QPair<QString, QString>, QList<ProjectExplorer::FileNode *> > filesToAdd;

    ResourceFile file(path().toString());
    if (file.load() == Core::IDocument::OpenResult::Success) {
        QSet<QPair<QString, QString > > prefixes;

        int prfxcount = file.prefixCount();
        for (int i = 0; i < prfxcount; ++i) {
            const QString &prefix = file.prefix(i);
            const QString &lang = file.lang(i);
            // ensure that we don't duplicate prefixes
            if (!prefixes.contains(qMakePair(prefix, lang))) {
                ProjectExplorer::FolderNode *fn = new ResourceFolderNode(file.prefix(i), file.lang(i), this);
                newFolderList << fn;

                prefixes.insert(qMakePair(prefix, lang));
            }

            QSet<QString> fileNames;
            int filecount = file.fileCount(i);
            for (int j = 0; j < filecount; ++j) {
                const QString &fileName = file.file(i, j);
                QString alias = file.alias(i, j);
                if (alias.isEmpty())
                    alias = path().toFileInfo().absoluteDir().relativeFilePath(fileName);
                if (fileNames.contains(fileName)) {
                    // The file name is duplicated, skip it
                    // Note: this is wrong, but the qrceditor doesn't allow it either
                    // only aliases need to be unique
                } else {
                    QString prefixWithSlash = prefix;
                    if (!prefixWithSlash.endsWith(QLatin1Char('/')))
                        prefixWithSlash.append(QLatin1Char('/'));
                    const QString qrcPath = QDir::cleanPath(prefixWithSlash + alias);
                    fileNames.insert(fileName);
                    filesToAdd[qMakePair(prefix, lang)]
                            << new ResourceFileNode(Utils::FileName::fromString(fileName),
                                                    qrcPath, this);
                }

            }
        }
    }

    QList<ProjectExplorer::FolderNode *> oldFolderList = subFolderNodes();
    QList<ProjectExplorer::FolderNode *> foldersToAdd;
    QList<ProjectExplorer::FolderNode *> foldersToRemove;

    std::sort(oldFolderList.begin(), oldFolderList.end(), sortByPrefixAndLang);
    std::sort(newFolderList.begin(), newFolderList.end(), sortByPrefixAndLang);

    ProjectExplorer::compareSortedLists(oldFolderList, newFolderList, foldersToRemove, foldersToAdd, sortByPrefixAndLang);

    removeFolderNodes(foldersToRemove);
    addFolderNodes(foldersToAdd);

    // delete nodes that weren't added
    qDeleteAll(ProjectExplorer::subtractSortedList(newFolderList, foldersToAdd, sortByPrefixAndLang));

    foreach (FolderNode *fn, subFolderNodes()) {
        ResourceFolderNode *rn = static_cast<ResourceFolderNode *>(fn);
        rn->updateFiles(filesToAdd.value(qMakePair(rn->prefix(), rn->lang())));
    }
}

QList<ProjectExplorer::ProjectAction> ResourceTopLevelNode::supportedActions(ProjectExplorer::Node *node) const
{
    if (node != this)
        return QList<ProjectExplorer::ProjectAction>();
    return QList<ProjectExplorer::ProjectAction>()
            << ProjectExplorer::AddNewFile
            << ProjectExplorer::AddExistingFile
            << ProjectExplorer::AddExistingDirectory
            << ProjectExplorer::HidePathActions
            << ProjectExplorer::Rename;
}

bool ResourceTopLevelNode::addFiles(const QStringList &filePaths, QStringList *notAdded)
{
    return addFilesToResource(path(), filePaths, notAdded, QLatin1String("/"), QString());
}

bool ResourceTopLevelNode::removeFiles(const QStringList &filePaths, QStringList *notRemoved)
{
    return parentFolderNode()->removeFiles(filePaths, notRemoved);
}

bool ResourceTopLevelNode::addPrefix(const QString &prefix, const QString &lang)
{
    ResourceFile file(path().toString());
    if (file.load() != Core::IDocument::OpenResult::Success)
        return false;
    int index = file.addPrefix(prefix, lang);
    if (index == -1)
        return false;
    Core::DocumentManager::expectFileChange(path().toString());
    file.save();
    Core::DocumentManager::unexpectFileChange(path().toString());

    return true;
}

bool ResourceTopLevelNode::removePrefix(const QString &prefix, const QString &lang)
{
    ResourceFile file(path().toString());
    if (file.load() != Core::IDocument::OpenResult::Success)
        return false;
    for (int i = 0; i < file.prefixCount(); ++i) {
        if (file.prefix(i) == prefix
                && file.lang(i) == lang) {
            file.removePrefix(i);
            Core::DocumentManager::expectFileChange(path().toString());
            file.save();
            Core::DocumentManager::unexpectFileChange(path().toString());
            return true;
        }
    }
    return false;
}

bool ResourceTopLevelNode::removeNonExistingFiles()
{
    ResourceFile file(path().toString());
    if (file.load() != Core::IDocument::OpenResult::Success)
        return false;

    QFileInfo fi;

    for (int i = 0; i < file.prefixCount(); ++i) {
        int fileCount = file.fileCount(i);
        for (int j = fileCount -1; j >= 0; --j) {
            fi.setFile(file.file(i, j));
            if (!fi.exists())
                file.removeFile(i, j);
        }
    }

    Core::DocumentManager::expectFileChange(path().toString());
    file.save();
    Core::DocumentManager::unexpectFileChange(path().toString());
    return true;
}

ProjectExplorer::FolderNode::AddNewInformation ResourceTopLevelNode::addNewInformation(const QStringList &files, Node *context) const
{
    QString name = QCoreApplication::translate("ResourceTopLevelNode", "%1 Prefix: %2")
            .arg(path().fileName())
            .arg(QLatin1Char('/'));

    int p = -1;
    if (priority(files)) { // images/* and qml/js mimetypes
        p = 110;
        if (context == this)
            p = 120;
        else if (projectNode() == context)
            p = 150; // steal from our project node
        // The ResourceFolderNode '/' defers to us, as otherwise
        // two nodes would be responsible for '/'
        // Thus also return a high priority for it
        if (ResourceFolderNode *rfn = dynamic_cast<ResourceFolderNode *>(context))
            if (rfn->prefix() == QLatin1String("/") && rfn->parentFolderNode() == this)
                p = 120;
    }

    return AddNewInformation(name, p);
}

bool ResourceTopLevelNode::showInSimpleTree() const
{
    return true;
}

ResourceFolderNode::ResourceFolderNode(const QString &prefix, const QString &lang, ResourceTopLevelNode *parent)
    : ProjectExplorer::FolderNode(Utils::FileName(parent->path()).appendPath(prefix)),
      // TOOD Why add existing directory doesn't work
      m_topLevelNode(parent),
      m_prefix(prefix),
      m_lang(lang)
{

}

ResourceFolderNode::~ResourceFolderNode()
{

}

QList<ProjectExplorer::ProjectAction> ResourceFolderNode::supportedActions(ProjectExplorer::Node *node) const
{
    Q_UNUSED(node)
    QList<ProjectExplorer::ProjectAction> actions;
    actions << ProjectExplorer::AddNewFile
            << ProjectExplorer::AddExistingFile
            << ProjectExplorer::AddExistingDirectory
            << ProjectExplorer::RemoveFile
            << ProjectExplorer::Rename // Note: only works for the filename, works akwardly for relative file paths
            << ProjectExplorer::HidePathActions; // hides open terminal etc.

    // if the prefix is '/' (without lang) hide this node in add new dialog,
    // as the ResouceTopLevelNode is always shown for the '/' prefix
    if (m_prefix == QLatin1String("/") && m_lang.isEmpty())
        actions << ProjectExplorer::InheritedFromParent;

    return actions;
}

bool ResourceFolderNode::addFiles(const QStringList &filePaths, QStringList *notAdded)
{
    return addFilesToResource(m_topLevelNode->path(), filePaths, notAdded, m_prefix, m_lang);
}

bool ResourceFolderNode::removeFiles(const QStringList &filePaths, QStringList *notRemoved)
{
    if (notRemoved)
        *notRemoved = filePaths;
    ResourceFile file(m_topLevelNode->path().toString());
    if (file.load() != Core::IDocument::OpenResult::Success)
        return false;
    int index = file.indexOfPrefix(m_prefix, m_lang);
    if (index == -1)
        return false;
    for (int j = 0; j < file.fileCount(index); ++j) {
        QString fileName = file.file(index, j);
        if (!filePaths.contains(fileName))
            continue;
        if (notRemoved)
            notRemoved->removeOne(fileName);
        file.removeFile(index, j);
        --j;
    }
    Core::DocumentManager::expectFileChange(m_topLevelNode->path().toString());
    file.save();
    Core::DocumentManager::unexpectFileChange(m_topLevelNode->path().toString());

    return true;
}

bool ResourceFolderNode::renameFile(const QString &filePath, const QString &newFilePath)
{
    ResourceFile file(m_topLevelNode->path().toString());
    if (file.load() != Core::IDocument::OpenResult::Success)
        return false;
    int index = file.indexOfPrefix(m_prefix, m_lang);
    if (index == -1)
        return false;

    for (int j = 0; j < file.fileCount(index); ++j) {
        if (file.file(index, j) == filePath) {
            file.replaceFile(index, j, newFilePath);
            Core::DocumentManager::expectFileChange(m_topLevelNode->path().toString());
            file.save();
            Core::DocumentManager::unexpectFileChange(m_topLevelNode->path().toString());
            return true;
        }
    }

    return false;
}

bool ResourceFolderNode::renamePrefix(const QString &prefix, const QString &lang)
{
    ResourceFile file(m_topLevelNode->path().toString());
    if (file.load() != Core::IDocument::OpenResult::Success)
        return false;
    int index = file.indexOfPrefix(m_prefix, m_lang);
    if (index == -1)
        return false;

    if (!file.replacePrefixAndLang(index, prefix, lang))
        return false;

    Core::DocumentManager::expectFileChange(m_topLevelNode->path().toString());
    file.save();
    Core::DocumentManager::unexpectFileChange(m_topLevelNode->path().toString());
    return true;
}

ProjectExplorer::FolderNode::AddNewInformation ResourceFolderNode::addNewInformation(const QStringList &files, Node *context) const
{
    QString name = QCoreApplication::translate("ResourceTopLevelNode", "%1 Prefix: %2")
            .arg(m_topLevelNode->path().fileName())
            .arg(displayName());

    int p = -1; // never the default
    if (priority(files)) { // image/* and qml/js mimetypes
        p = 105; // prefer against .pro and .pri files
        if (context == this)
            p = 120;
    }

    return AddNewInformation(name, p);
}

QString ResourceFolderNode::displayName() const
{
    if (m_lang.isEmpty())
        return m_prefix;
    return m_prefix + QLatin1String(" (") + m_lang + QLatin1Char(')');
}

QString ResourceFolderNode::prefix() const
{
    return m_prefix;
}

QString ResourceFolderNode::lang() const
{
    return m_lang;
}

ResourceTopLevelNode *ResourceFolderNode::resourceNode() const
{
    return m_topLevelNode;
}

void ResourceFolderNode::updateFiles(QList<ProjectExplorer::FileNode *> newList)
{
    QList<ProjectExplorer::FileNode *> oldList = fileNodes();
    QList<ProjectExplorer::FileNode *> filesToAdd;
    QList<ProjectExplorer::FileNode *> filesToRemove;

    std::sort(oldList.begin(), oldList.end(), sortNodesByPath);
    std::sort(newList.begin(), newList.end(), sortNodesByPath);

    ProjectExplorer::compareSortedLists(oldList, newList, filesToRemove, filesToAdd, sortNodesByPath);

    removeFileNodes(filesToRemove);
    addFileNodes(filesToAdd);

    qDeleteAll(ProjectExplorer::subtractSortedList(newList, filesToAdd, sortNodesByPath));
}

ResourceFileWatcher::ResourceFileWatcher(ResourceTopLevelNode *node)
    : IDocument(0), m_node(node)
{
    setId("ResourceNodeWatcher");
    setMimeType(QLatin1String(ResourceEditor::Constants::C_RESOURCE_MIMETYPE));
    setFilePath(node->path());
}

bool ResourceFileWatcher::save(QString *errorString, const QString &fileName, bool autoSave)
{
    Q_UNUSED(errorString);
    Q_UNUSED(fileName);
    Q_UNUSED(autoSave);
    return false;
}

QString ResourceFileWatcher::defaultPath() const
{
    return QString();
}

QString ResourceFileWatcher::suggestedFileName() const
{
    return QString();
}

bool ResourceFileWatcher::isModified() const
{
    return false;
}

bool ResourceFileWatcher::isSaveAsAllowed() const
{
    return false;
}

Core::IDocument::ReloadBehavior ResourceFileWatcher::reloadBehavior(ChangeTrigger state, ChangeType type) const
{
    Q_UNUSED(state)
    Q_UNUSED(type)
    return BehaviorSilent;
}

bool ResourceFileWatcher::reload(QString *errorString, ReloadFlag flag, ChangeType type)
{
    Q_UNUSED(errorString)
    Q_UNUSED(flag)
    if (type == TypePermissions)
        return true;
    m_node->update();
    return true;
}

ResourceFileNode::ResourceFileNode(const Utils::FileName &filePath, const QString &qrcPath, ResourceTopLevelNode *topLevel)
    : ProjectExplorer::FileNode(filePath, ProjectExplorer::UnknownFileType, false),
      m_qrcPath(qrcPath)

{
    QDir baseDir = topLevel->path().toFileInfo().absoluteDir();
    m_displayName = QDir(baseDir).relativeFilePath(filePath.toString());
}

QString ResourceFileNode::displayName() const
{
    return m_displayName;
}

QString ResourceFileNode::qrcPath() const
{
    return m_qrcPath;
}

QList<ProjectExplorer::ProjectAction> ResourceFileNode::supportedActions(ProjectExplorer::Node *node) const
{
    QList<ProjectExplorer::ProjectAction> actions = parentFolderNode()->supportedActions(node);
    actions.removeOne(ProjectExplorer::HidePathActions);
    return actions;
}
