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

#include "resourcenode.h"
#include "resourceeditorconstants.h"
#include "qrceditor/resourcefile_p.h"

#include <coreplugin/documentmanager.h>
#include <coreplugin/fileiconprovider.h>

#include <qmljstools/qmljstoolsconstants.h>

#include <utils/algorithm.h>
#include <utils/fileutils.h>
#include <utils/mimetypes/mimedatabase.h>
#include <utils/qtcassert.h>

#include <QCoreApplication>
#include <QDir>
#include <QDebug>

using namespace ResourceEditor;
using namespace ResourceEditor::Internal;

static bool hasPriority(const QStringList &files)
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

    Core::FileChangeBlocker changeGuard(resourceFile.toString());
    file.save();

    return true;
}

ResourceTopLevelNode::ResourceTopLevelNode(
        const Utils::FileName &filePath, const QString &contents,
        ProjectExplorer::FolderNode *parent)
    : ProjectExplorer::FolderNode(filePath)
{
    setIcon(Core::FileIconProvider::icon(filePath.toString()));
    if (contents.isEmpty()) {
        m_document = new ResourceFileWatcher(this);
        Core::DocumentManager::addDocument(m_document);
    } else {
        m_contents = contents;
        m_document = nullptr;
    }

    Utils::FileName base = parent->filePath();
    if (filePath.isChildOf(base))
        setDisplayName(filePath.relativeChildPath(base).toUserOutput());
    else
        setDisplayName(filePath.toUserOutput());
}

ResourceTopLevelNode::~ResourceTopLevelNode()
{
    if (m_document)
        Core::DocumentManager::removeDocument(m_document);
    delete m_document;
}

void ResourceTopLevelNode::addInternalNodes()
{
    ResourceFile file(filePath().toString(), m_contents);
    if (file.load() != Core::IDocument::OpenResult::Success)
        return;

    QMap<PrefixFolderLang, ProjectExplorer::FolderNode *> folderNodes;

    int prfxcount = file.prefixCount();
    for (int i = 0; i < prfxcount; ++i) {
        const QString &prefix = file.prefix(i);
        const QString &lang = file.lang(i);
        // ensure that we don't duplicate prefixes
        PrefixFolderLang prefixId(prefix, QString(), lang);
        if (!folderNodes.contains(prefixId)) {
            ProjectExplorer::FolderNode *fn = new ResourceFolderNode(file.prefix(i), file.lang(i), this);
            addNode(fn);
            folderNodes.insert(prefixId, fn);
        }
        ResourceFolderNode *currentPrefixNode = static_cast<ResourceFolderNode*>(folderNodes[prefixId]);

        QSet<QString> fileNames;
        int filecount = file.fileCount(i);
        for (int j = 0; j < filecount; ++j) {
            const QString &fileName = file.file(i, j);
            if (fileNames.contains(fileName)) {
                // The file name is duplicated, skip it
                // Note: this is wrong, but the qrceditor doesn't allow it either
                // only aliases need to be unique
                continue;
            }

            QString alias = file.alias(i, j);
            if (alias.isEmpty())
                alias = filePath().toFileInfo().absoluteDir().relativeFilePath(fileName);

            QString prefixWithSlash = prefix;
            if (!prefixWithSlash.endsWith(QLatin1Char('/')))
                prefixWithSlash.append(QLatin1Char('/'));

            const QString fullPath = QDir::cleanPath(alias);
            QStringList pathList = fullPath.split(QLatin1Char('/'));
            const QString displayName = pathList.last();
            pathList.removeLast(); // remove file name

            bool parentIsPrefix = true;

            QString parentFolderName;
            PrefixFolderLang folderId(prefix, QString(), lang);
            QStringList currentPathList;
            foreach (const QString &pathElement, pathList) {
                currentPathList << pathElement;
                const QString folderName = currentPathList.join(QLatin1Char('/'));
                folderId = PrefixFolderLang(prefix, folderName, lang);
                if (!folderNodes.contains(folderId)) {
                    const QString absoluteFolderName
                            = filePath().toFileInfo().absoluteDir().absoluteFilePath(
                                currentPathList.join(QLatin1Char('/')));
                    const Utils::FileName folderPath
                            = Utils::FileName::fromString(absoluteFolderName);
                    ProjectExplorer::FolderNode *newNode
                            = new SimpleResourceFolderNode(folderName, pathElement,
                                                           prefix, lang, folderPath,
                                                           this, currentPrefixNode);
                    folderNodes.insert(folderId, newNode);

                    PrefixFolderLang thisPrefixId = prefixId;
                    if (!parentIsPrefix)
                        thisPrefixId = PrefixFolderLang(prefix, parentFolderName, lang);
                    FolderNode *fn = folderNodes[thisPrefixId];
                    QTC_CHECK(fn);
                    if (fn)
                        fn->addNode(newNode);
                }
                parentIsPrefix = false;
                parentFolderName = folderName;
            }

            const QString qrcPath = QDir::cleanPath(prefixWithSlash + alias);
            fileNames.insert(fileName);
            FolderNode *fn = folderNodes[folderId];
            QTC_CHECK(fn);
            if (fn)
                fn->addNode(new ResourceFileNode(Utils::FileName::fromString(fileName),
                                                 qrcPath, displayName));
        }
    }
}

QString ResourceTopLevelNode::addFileFilter() const
{
    return QLatin1String("*.png; *.jpg; *.gif; *.svg; *.ico; *.qml; *.qml.ui");
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
    return addFilesToResource(filePath(), filePaths, notAdded, QLatin1String("/"), QString());
}

bool ResourceTopLevelNode::removeFiles(const QStringList &filePaths, QStringList *notRemoved)
{
    return parentFolderNode()->removeFiles(filePaths, notRemoved);
}

bool ResourceTopLevelNode::addPrefix(const QString &prefix, const QString &lang)
{
    ResourceFile file(filePath().toString());
    if (file.load() != Core::IDocument::OpenResult::Success)
        return false;
    int index = file.addPrefix(prefix, lang);
    if (index == -1)
        return false;
    Core::FileChangeBlocker changeGuard(filePath().toString());
    file.save();

    return true;
}

bool ResourceTopLevelNode::removePrefix(const QString &prefix, const QString &lang)
{
    ResourceFile file(filePath().toString());
    if (file.load() != Core::IDocument::OpenResult::Success)
        return false;
    for (int i = 0; i < file.prefixCount(); ++i) {
        if (file.prefix(i) == prefix
                && file.lang(i) == lang) {
            file.removePrefix(i);
            Core::FileChangeBlocker changeGuard(filePath().toString());
            file.save();
            return true;
        }
    }
    return false;
}

bool ResourceTopLevelNode::removeNonExistingFiles()
{
    ResourceFile file(filePath().toString());
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

    Core::FileChangeBlocker changeGuard(filePath().toString());
    file.save();
    return true;
}

ProjectExplorer::FolderNode::AddNewInformation ResourceTopLevelNode::addNewInformation(const QStringList &files, Node *context) const
{
    QString name = QCoreApplication::translate("ResourceTopLevelNode", "%1 Prefix: %2")
            .arg(filePath().fileName())
            .arg(QLatin1Char('/'));

    int p = -1;
    if (hasPriority(files)) { // images/* and qml/js mimetypes
        p = 110;
        if (context == this)
            p = 120;
        else if (parentProjectNode() == context)
            p = 150; // steal from our project node
        // The ResourceFolderNode '/' defers to us, as otherwise
        // two nodes would be responsible for '/'
        // Thus also return a high priority for it
        if (ResourceFolderNode *rfn = dynamic_cast<ResourceFolderNode *>(context))
            if (rfn->prefix() == QLatin1String("/") && rfn->parentFolderNode() == this)
                p = 120;
        if (SimpleResourceFolderNode *rfn = dynamic_cast<SimpleResourceFolderNode *>(context))
            if (rfn->prefix() == QLatin1String("/") && rfn->resourceNode() == this)
                p = 120;
    }

    return AddNewInformation(name, p);
}

bool ResourceTopLevelNode::showInSimpleTree() const
{
    return true;
}

ResourceFolderNode::ResourceFolderNode(const QString &prefix, const QString &lang, ResourceTopLevelNode *parent)
    : ProjectExplorer::FolderNode(Utils::FileName(parent->filePath()).appendPath(prefix)),
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
            << ProjectExplorer::DuplicateFile
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
    return addFilesToResource(m_topLevelNode->filePath(), filePaths, notAdded, m_prefix, m_lang);
}

bool ResourceFolderNode::removeFiles(const QStringList &filePaths, QStringList *notRemoved)
{
    if (notRemoved)
        *notRemoved = filePaths;
    ResourceFile file(m_topLevelNode->filePath().toString());
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
    Core::FileChangeBlocker changeGuard(m_topLevelNode->filePath().toString());
    file.save();

    return true;
}

// QTCREATORBUG-15280
bool ResourceFolderNode::canRenameFile(const QString &filePath, const QString &newFilePath)
{
    Q_UNUSED(newFilePath)

    bool fileEntryExists = false;
    ResourceFile file(m_topLevelNode->filePath().toString());

    int index = (file.load() != Core::IDocument::OpenResult::Success) ? -1 :file.indexOfPrefix(m_prefix, m_lang);
    if (index != -1) {
        for (int j = 0; j < file.fileCount(index); ++j) {
            if (file.file(index, j) == filePath) {
                fileEntryExists = true;
                break;
            }
        }
    }

    return fileEntryExists;
}

bool ResourceFolderNode::renameFile(const QString &filePath, const QString &newFilePath)
{
    ResourceFile file(m_topLevelNode->filePath().toString());
    if (file.load() != Core::IDocument::OpenResult::Success)
        return false;
    int index = file.indexOfPrefix(m_prefix, m_lang);
    if (index == -1)
        return false;

    for (int j = 0; j < file.fileCount(index); ++j) {
        if (file.file(index, j) == filePath) {
            file.replaceFile(index, j, newFilePath);
            Core::FileChangeBlocker changeGuard(m_topLevelNode->filePath().toString());
            file.save();
            return true;
        }
    }

    return false;
}

bool ResourceFolderNode::renamePrefix(const QString &prefix, const QString &lang)
{
    ResourceFile file(m_topLevelNode->filePath().toString());
    if (file.load() != Core::IDocument::OpenResult::Success)
        return false;
    int index = file.indexOfPrefix(m_prefix, m_lang);
    if (index == -1)
        return false;

    if (!file.replacePrefixAndLang(index, prefix, lang))
        return false;

    Core::FileChangeBlocker changeGuard(m_topLevelNode->filePath().toString());
    file.save();
    return true;
}

ProjectExplorer::FolderNode::AddNewInformation ResourceFolderNode::addNewInformation(const QStringList &files, Node *context) const
{
    QString name = QCoreApplication::translate("ResourceTopLevelNode", "%1 Prefix: %2")
            .arg(m_topLevelNode->filePath().fileName())
            .arg(displayName());

    int p = -1; // never the default
    if (hasPriority(files)) { // image/* and qml/js mimetypes
        p = 105; // prefer against .pro and .pri files
        if (context == this)
            p = 120;

        if (SimpleResourceFolderNode *sfn = dynamic_cast<SimpleResourceFolderNode *>(context)) {
            if (sfn->prefixNode() == this)
                p = 120;
        }
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

ResourceFileWatcher::ResourceFileWatcher(ResourceTopLevelNode *node)
    : IDocument(0), m_node(node)
{
    setId("ResourceNodeWatcher");
    setMimeType(QLatin1String(ResourceEditor::Constants::C_RESOURCE_MIMETYPE));
    setFilePath(node->filePath());
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
    m_node->makeEmpty();
    m_node->addInternalNodes();
    return true;
}

ResourceFileNode::ResourceFileNode(const Utils::FileName &filePath, const QString &qrcPath, const QString &displayName)
    : ProjectExplorer::FileNode(filePath, ProjectExplorer::FileType::Unknown, false)
    , m_qrcPath(qrcPath)
    , m_displayName(displayName)
{
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

QString SimpleResourceFolderNode::displayName() const
{
    if (!m_displayName.isEmpty())
        return m_displayName;
    return FolderNode::displayName();
}

SimpleResourceFolderNode::SimpleResourceFolderNode(const QString &afolderName, const QString &displayName,
                                   const QString &prefix, const QString &lang,
                                   Utils::FileName absolutePath, ResourceTopLevelNode *topLevel, ResourceFolderNode *prefixNode)
    : ProjectExplorer::FolderNode(absolutePath)
    , m_folderName(afolderName)
    , m_displayName(displayName)
    , m_prefix(prefix)
    , m_lang(lang)
    , m_topLevelNode(topLevel)
    , m_prefixNode(prefixNode)
{

}

QList<ProjectExplorer::ProjectAction> SimpleResourceFolderNode::supportedActions(ProjectExplorer::Node *node) const
{
    Q_UNUSED(node)
    QList<ProjectExplorer::ProjectAction> actions;
    actions << ProjectExplorer::AddNewFile
            << ProjectExplorer::AddExistingFile
            << ProjectExplorer::AddExistingDirectory
            << ProjectExplorer::RemoveFile
            << ProjectExplorer::DuplicateFile
            << ProjectExplorer::Rename // Note: only works for the filename, works akwardly for relative file paths
            << ProjectExplorer::InheritedFromParent; // do not add to list of projects when adding new file

    return actions;
}

bool SimpleResourceFolderNode::addFiles(const QStringList &filePaths, QStringList *notAdded)
{
    return addFilesToResource(m_topLevelNode->filePath(), filePaths, notAdded, m_prefix, m_lang);
}

bool SimpleResourceFolderNode::removeFiles(const QStringList &filePaths, QStringList *notRemoved)
{
    if (notRemoved)
        *notRemoved = filePaths;
    ResourceFile file(m_topLevelNode->filePath().toString());
    if (file.load() != Core::IDocument::OpenResult::Success)
        return false;
    int index = file.indexOfPrefix(m_prefix, m_lang);
    if (index == -1)
        return false;
    for (int j = 0; j < file.fileCount(index); ++j) {
        const QString fileName = file.file(index, j);
        if (!filePaths.contains(fileName))
            continue;
        if (notRemoved)
            notRemoved->removeOne(fileName);
        file.removeFile(index, j);
        --j;
    }
    Core::FileChangeBlocker changeGuard(m_topLevelNode->filePath().toString());
    file.save();

    return true;
}

bool SimpleResourceFolderNode::renameFile(const QString &filePath, const QString &newFilePath)
{
    ResourceFile file(m_topLevelNode->filePath().toString());
    if (file.load() != Core::IDocument::OpenResult::Success)
        return false;
    int index = file.indexOfPrefix(m_prefix, m_lang);
    if (index == -1)
        return false;

    for (int j = 0; j < file.fileCount(index); ++j) {
        if (file.file(index, j) == filePath) {
            file.replaceFile(index, j, newFilePath);
            Core::FileChangeBlocker changeGuard(m_topLevelNode->filePath().toString());
            file.save();
            return true;
        }
    }

    return false;
}

QString SimpleResourceFolderNode::prefix() const
{
    return m_prefix;
}

ResourceTopLevelNode *SimpleResourceFolderNode::resourceNode() const
{
    return m_topLevelNode;
}

ResourceFolderNode *SimpleResourceFolderNode::prefixNode() const
{
    return m_prefixNode;
}
