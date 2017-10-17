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

using namespace Core;
using namespace ProjectExplorer;
using namespace Utils;

using namespace ResourceEditor::Internal;

namespace ResourceEditor {
namespace Internal {

class ResourceFileWatcher : public IDocument
{
public:
    ResourceFileWatcher(ResourceTopLevelNode *node)
        : IDocument(nullptr), m_node(node)
    {
        setId("ResourceNodeWatcher");
        setMimeType(ResourceEditor::Constants::C_RESOURCE_MIMETYPE);
        setFilePath(node->filePath());
    }

    ReloadBehavior reloadBehavior(ChangeTrigger, ChangeType) const final
    {
        return BehaviorSilent;
    }

    bool reload(QString *, ReloadFlag, ChangeType type) final
    {
        if (type == TypePermissions)
            return true;
        FolderNode *parent = m_node->parentFolderNode();
        QTC_ASSERT(parent, return false);
        auto newNode = new ResourceTopLevelNode(m_node->filePath(), false, m_node->contents(), parent);
        m_node->parentFolderNode()->replaceSubtree(m_node, newNode);
        return true;
    }

private:
    ResourceTopLevelNode *m_node;
};

class PrefixFolderLang
{
public:
    PrefixFolderLang(QString prefix, QString folder, QString lang)
        : m_prefix(prefix)
        , m_folder(folder)
        , m_lang(lang)
    {}

    bool operator<(const PrefixFolderLang &other) const
    {
        if (m_prefix != other.m_prefix)
            return m_prefix < other.m_prefix;
        if (m_folder != other.m_folder)
            return m_folder < other.m_folder;
        if (m_lang != other.m_lang)
            return m_lang < other.m_lang;
        return false;
    }
private:
    QString m_prefix;
    QString m_folder;
    QString m_lang;
};

static bool hasPriority(const QStringList &files)
{
    if (files.isEmpty())
        return false;
    QString type = Utils::mimeTypeForFile(files.at(0)).name();
    if (type.startsWith(QLatin1String("image/"))
            || type == QLatin1String(QmlJSTools::Constants::QML_MIMETYPE)
            || type == QLatin1String(QmlJSTools::Constants::QMLUI_MIMETYPE)
            || type == QLatin1String(QmlJSTools::Constants::JS_MIMETYPE))
        return true;
    return false;
}

static bool addFilesToResource(const FileName &resourceFile,
                               const QStringList &filePaths,
                               QStringList *notAdded,
                               const QString &prefix,
                               const QString &lang)
{
    if (notAdded)
        *notAdded = filePaths;

    ResourceFile file(resourceFile.toString());
    if (file.load() != IDocument::OpenResult::Success)
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

    file.save();

    return true;
}

class SimpleResourceFolderNode : public FolderNode
{
    friend class ResourceEditor::ResourceTopLevelNode;
public:
    SimpleResourceFolderNode(const QString &afolderName, const QString &displayName,
                     const QString &prefix, const QString &lang, FileName absolutePath,
                     ResourceTopLevelNode *topLevel, ResourceFolderNode *prefixNode);

    QString displayName() const final;
    bool supportsAction(ProjectAction, const Node *node) const final;
    bool addFiles(const QStringList &filePaths, QStringList *notAdded) final;
    bool removeFiles(const QStringList &filePaths, QStringList *notRemoved) final;
    bool renameFile(const QString &filePath, const QString &newFilePath) final;

    QString prefix() const { return m_prefix; }
    ResourceTopLevelNode *resourceNode() const { return m_topLevelNode; }
    ResourceFolderNode *prefixNode() const { return m_prefixNode; }

private:
    QString m_folderName;
    QString m_displayName;
    QString m_prefix;
    QString m_lang;
    ResourceTopLevelNode *m_topLevelNode;
    ResourceFolderNode *m_prefixNode;
};

QString SimpleResourceFolderNode::displayName() const
{
    if (!m_displayName.isEmpty())
        return m_displayName;
    return FolderNode::displayName();
}

SimpleResourceFolderNode::SimpleResourceFolderNode(const QString &afolderName, const QString &displayName,
                                   const QString &prefix, const QString &lang,
                                   FileName absolutePath, ResourceTopLevelNode *topLevel, ResourceFolderNode *prefixNode)
    : FolderNode(absolutePath)
    , m_folderName(afolderName)
    , m_displayName(displayName)
    , m_prefix(prefix)
    , m_lang(lang)
    , m_topLevelNode(topLevel)
    , m_prefixNode(prefixNode)
{

}

bool SimpleResourceFolderNode::supportsAction(ProjectAction action, const Node *) const
{
    return action == AddNewFile
        || action == AddExistingFile
        || action == AddExistingDirectory
        || action == RemoveFile
        || action == DuplicateFile
        || action == Rename // Note: only works for the filename, works akwardly for relative file paths
        || action == InheritedFromParent; // Do not add to list of projects when adding new file
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
    if (file.load() != IDocument::OpenResult::Success)
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
    FileChangeBlocker changeGuard(m_topLevelNode->filePath().toString());
    file.save();

    return true;
}

bool SimpleResourceFolderNode::renameFile(const QString &filePath, const QString &newFilePath)
{
    ResourceFile file(m_topLevelNode->filePath().toString());
    if (file.load() != IDocument::OpenResult::Success)
        return false;
    int index = file.indexOfPrefix(m_prefix, m_lang);
    if (index == -1)
        return false;

    for (int j = 0; j < file.fileCount(index); ++j) {
        if (file.file(index, j) == filePath) {
            file.replaceFile(index, j, newFilePath);
            FileChangeBlocker changeGuard(m_topLevelNode->filePath().toString());
            file.save();
            return true;
        }
    }

    return false;
}

} // Internal

ResourceTopLevelNode::ResourceTopLevelNode(const FileName &filePath, bool generated,
                                           const QString &contents, FolderNode *parent)
    : FolderNode(filePath)
{
    setIsGenerated(generated);
    setIcon(FileIconProvider::icon(filePath.toString()));
    setPriority(Node::DefaultFilePriority);
    setListInProject(true);
    if (!filePath.isEmpty()) {
        QFileInfo fi = filePath.toFileInfo();
        if (fi.isFile() && fi.isReadable()) {
            m_document = new ResourceFileWatcher(this);
            DocumentManager::addDocument(m_document);
        }
    } else {
        m_contents = contents;
    }

    FileName base = parent->filePath();
    if (filePath.isChildOf(base))
        setDisplayName(filePath.relativeChildPath(base).toUserOutput());
    else
        setDisplayName(filePath.toUserOutput());

    addInternalNodes();
}

ResourceTopLevelNode::~ResourceTopLevelNode()
{
    if (m_document)
        DocumentManager::removeDocument(m_document);
    delete m_document;
}

void ResourceTopLevelNode::addInternalNodes()
{
    ResourceFile file(filePath().toString(), m_contents);
    if (file.load() != IDocument::OpenResult::Success)
        return;

    QMap<PrefixFolderLang, FolderNode *> folderNodes;

    int prfxcount = file.prefixCount();
    for (int i = 0; i < prfxcount; ++i) {
        const QString &prefix = file.prefix(i);
        const QString &lang = file.lang(i);
        // ensure that we don't duplicate prefixes
        PrefixFolderLang prefixId(prefix, QString(), lang);
        if (!folderNodes.contains(prefixId)) {
            FolderNode *fn = new ResourceFolderNode(file.prefix(i), file.lang(i), this);
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
                    const FileName folderPath = FileName::fromString(absoluteFolderName);
                    FolderNode *newNode
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
                fn->addNode(new ResourceFileNode(FileName::fromString(fileName),
                                                 qrcPath, displayName));
        }
    }
}

QString ResourceTopLevelNode::addFileFilter() const
{
    return QLatin1String("*.png; *.jpg; *.gif; *.svg; *.ico; *.qml; *.qml.ui");
}

bool ResourceTopLevelNode::supportsAction(ProjectAction action, const Node *node) const
{
    if (node != this)
        return false;
    return action == AddNewFile
        || action == AddExistingFile
        || action == AddExistingDirectory
        || action == HidePathActions
        || action == Rename;
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
    if (file.load() != IDocument::OpenResult::Success)
        return false;
    int index = file.addPrefix(prefix, lang);
    if (index == -1)
        return false;
    file.save();

    return true;
}

bool ResourceTopLevelNode::removePrefix(const QString &prefix, const QString &lang)
{
    ResourceFile file(filePath().toString());
    if (file.load() != IDocument::OpenResult::Success)
        return false;
    for (int i = 0; i < file.prefixCount(); ++i) {
        if (file.prefix(i) == prefix
                && file.lang(i) == lang) {
            file.removePrefix(i);
            file.save();
            return true;
        }
    }
    return false;
}

bool ResourceTopLevelNode::removeNonExistingFiles()
{
    ResourceFile file(filePath().toString());
    if (file.load() != IDocument::OpenResult::Success)
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

    file.save();
    return true;
}

FolderNode::AddNewInformation ResourceTopLevelNode::addNewInformation(const QStringList &files, Node *context) const
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
        if (auto rfn = dynamic_cast<ResourceFolderNode *>(context))
            if (rfn->prefix() == QLatin1String("/") && rfn->parentFolderNode() == this)
                p = 120;
        if (auto rfn = dynamic_cast<SimpleResourceFolderNode *>(context))
            if (rfn->prefix() == QLatin1String("/") && rfn->resourceNode() == this)
                p = 120;
    }

    return AddNewInformation(name, p);
}

bool ResourceTopLevelNode::showInSimpleTree() const
{
    return true;
}

bool ResourceTopLevelNode::showWhenEmpty() const
{
    return true;
}

ResourceFolderNode::ResourceFolderNode(const QString &prefix, const QString &lang, ResourceTopLevelNode *parent)
    : FolderNode(FileName(parent->filePath()).appendPath(prefix)),
      // TOOD Why add existing directory doesn't work
      m_topLevelNode(parent),
      m_prefix(prefix),
      m_lang(lang)
{

}

ResourceFolderNode::~ResourceFolderNode()
{

}

bool ResourceFolderNode::supportsAction(ProjectAction action, const Node *node) const
{
    Q_UNUSED(node)

    if (action == InheritedFromParent) {
        // if the prefix is '/' (without lang) hide this node in add new dialog,
        // as the ResouceTopLevelNode is always shown for the '/' prefix
        return m_prefix == QLatin1String("/") && m_lang.isEmpty();
    }

    return action == AddNewFile
        || action == AddExistingFile
        || action == AddExistingDirectory
        || action == RemoveFile
        || action == DuplicateFile
        || action == Rename // Note: only works for the filename, works akwardly for relative file paths
        || action == HidePathActions; // hides open terminal etc.
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
    if (file.load() != IDocument::OpenResult::Success)
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
    file.save();

    return true;
}

// QTCREATORBUG-15280
bool ResourceFolderNode::canRenameFile(const QString &filePath, const QString &newFilePath)
{
    Q_UNUSED(newFilePath)

    bool fileEntryExists = false;
    ResourceFile file(m_topLevelNode->filePath().toString());

    int index = (file.load() != IDocument::OpenResult::Success) ? -1 :file.indexOfPrefix(m_prefix, m_lang);
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
    if (file.load() != IDocument::OpenResult::Success)
        return false;
    int index = file.indexOfPrefix(m_prefix, m_lang);
    if (index == -1)
        return false;

    for (int j = 0; j < file.fileCount(index); ++j) {
        if (file.file(index, j) == filePath) {
            file.replaceFile(index, j, newFilePath);
            file.save();
            return true;
        }
    }

    return false;
}

bool ResourceFolderNode::renamePrefix(const QString &prefix, const QString &lang)
{
    ResourceFile file(m_topLevelNode->filePath().toString());
    if (file.load() != IDocument::OpenResult::Success)
        return false;
    int index = file.indexOfPrefix(m_prefix, m_lang);
    if (index == -1)
        return false;

    if (!file.replacePrefixAndLang(index, prefix, lang))
        return false;

    file.save();
    return true;
}

FolderNode::AddNewInformation ResourceFolderNode::addNewInformation(const QStringList &files, Node *context) const
{
    QString name = QCoreApplication::translate("ResourceTopLevelNode", "%1 Prefix: %2")
            .arg(m_topLevelNode->filePath().fileName())
            .arg(displayName());

    int p = -1; // never the default
    if (hasPriority(files)) { // image/* and qml/js mimetypes
        p = 105; // prefer against .pro and .pri files
        if (context == this)
            p = 120;

        if (auto sfn = dynamic_cast<SimpleResourceFolderNode *>(context)) {
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

ResourceFileNode::ResourceFileNode(const FileName &filePath, const QString &qrcPath, const QString &displayName)
    : FileNode(filePath, FileNode::fileTypeForFileName(filePath), false)
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

bool ResourceFileNode::supportsAction(ProjectAction action, const Node *node) const
{
    if (action == HidePathActions)
        return false;
    return parentFolderNode()->supportsAction(action, node);
}

} // ResourceEditor
