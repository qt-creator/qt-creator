// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "resourcenode.h"

#include "resourceeditorconstants.h"
#include "resourceeditortr.h"
#include "qrceditor/resourcefile_p.h"

#include <coreplugin/documentmanager.h>

#include <qmljstools/qmljstoolsconstants.h>

#include <utils/algorithm.h>
#include <utils/fileutils.h>
#include <utils/fsengine/fileiconprovider.h>
#include <utils/mimeutils.h>
#include <utils/qtcassert.h>
#include <utils/threadutils.h>

#include <QCoreApplication>
#include <QDir>
#include <QDebug>

#include <limits>

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
        Q_UNUSED(type)
        FolderNode *parent = m_node->parentFolderNode();
        QTC_ASSERT(parent, return false);
        parent->replaceSubtree(m_node, std::make_unique<ResourceTopLevelNode>(
                                   m_node->filePath(), parent->filePath(), m_node->contents()));
        return true;
    }

private:
    ResourceTopLevelNode *m_node;
};

class PrefixFolderLang
{
public:
    PrefixFolderLang(const QString &prefix, const QString &folder, const QString &lang)
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

static int getPriorityFromContextNode(const ProjectExplorer::Node *resourceNode,
                                      const ProjectExplorer::Node *contextNode)
{
    if (contextNode == resourceNode)
        return std::numeric_limits<int>::max();
    for (const ProjectExplorer::Node *n = contextNode; n; n = n->parentFolderNode()) {
        if (n == resourceNode)
            return std::numeric_limits<int>::max() - 1;
    }
    return -1;
}

static bool hasPriority(const FilePaths &files)
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

static bool addFilesToResource(const FilePath &resourceFile,
                               const FilePaths &filePaths,
                               FilePaths *notAdded,
                               const QString &prefix,
                               const QString &lang)
{
    if (notAdded)
        *notAdded = filePaths;

    ResourceFile file(resourceFile);
    if (file.load() != IDocument::OpenResult::Success)
        return false;

    int index = file.indexOfPrefix(prefix, lang);
    if (index == -1)
        index = file.addPrefix(prefix, lang);

    if (notAdded)
        notAdded->clear();
    for (const FilePath &path : filePaths) {
        if (file.contains(index, path.toString())) {
            if (notAdded)
                *notAdded << path;
        } else {
            file.addFile(index, path.toString());
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
                     const QString &prefix, const QString &lang, FilePath absolutePath,
                     ResourceTopLevelNode *topLevel, ResourceFolderNode *prefixNode);

    bool supportsAction(ProjectAction, const Node *node) const final;
    bool addFiles(const Utils::FilePaths &filePaths, Utils::FilePaths *notAdded) final;
    RemovedFilesFromProject removeFiles(const Utils::FilePaths &filePaths,
                                        Utils::FilePaths *notRemoved) final;
    bool canRenameFile(const Utils::FilePath &oldFilePath, const Utils::FilePath &newFilePath) override;
    bool renameFile(const Utils::FilePath &oldFilePath, const Utils::FilePath &newFilePath) final;

    QString prefix() const { return m_prefix; }
    ResourceTopLevelNode *resourceNode() const { return m_topLevelNode; }
    ResourceFolderNode *prefixNode() const { return m_prefixNode; }

private:
    QString m_folderName;
    QString m_prefix;
    QString m_lang;
    ResourceTopLevelNode *m_topLevelNode;
    ResourceFolderNode *m_prefixNode;
};

SimpleResourceFolderNode::SimpleResourceFolderNode(const QString &afolderName, const QString &displayName,
                                   const QString &prefix, const QString &lang,
                                   FilePath absolutePath, ResourceTopLevelNode *topLevel, ResourceFolderNode *prefixNode)
    : FolderNode(absolutePath)
    , m_folderName(afolderName)
    , m_prefix(prefix)
    , m_lang(lang)
    , m_topLevelNode(topLevel)
    , m_prefixNode(prefixNode)
{
    setDisplayName(displayName);
}

bool SimpleResourceFolderNode::supportsAction(ProjectAction action, const Node *) const
{
    return action == AddNewFile
        || action == AddExistingFile
        || action == AddExistingDirectory
        || action == RemoveFile
        || action == Rename // Note: only works for the filename, works akwardly for relative file paths
        || action == InheritedFromParent; // Do not add to list of projects when adding new file
}

bool SimpleResourceFolderNode::addFiles(const FilePaths &filePaths, FilePaths *notAdded)
{
    return addFilesToResource(m_topLevelNode->filePath(), filePaths, notAdded, m_prefix, m_lang);
}

RemovedFilesFromProject SimpleResourceFolderNode::removeFiles(const FilePaths &filePaths,
                                                              FilePaths *notRemoved)
{
    return prefixNode()->removeFiles(filePaths, notRemoved);
}

bool SimpleResourceFolderNode::canRenameFile(const FilePath &oldFilePath,
                                             const FilePath &newFilePath)
{
    return prefixNode()->canRenameFile(oldFilePath, newFilePath);
}

bool SimpleResourceFolderNode::renameFile(const FilePath &oldFilePath, const FilePath &newFilePath)
{
    return prefixNode()->renameFile(oldFilePath, newFilePath);
}

} // Internal

ResourceTopLevelNode::ResourceTopLevelNode(const FilePath &filePath,
                                           const FilePath &base,
                                           const QString &contents)
    : FolderNode(filePath)
{
    setIcon([filePath] { return FileIconProvider::icon(filePath); });
    setPriority(Node::DefaultFilePriority);
    setListInProject(true);
    setAddFileFilter("*.png; *.jpg; *.gif; *.svg; *.ico; *.qml; *.qml.ui");
    setShowWhenEmpty(true);

    if (!filePath.isEmpty()) {
        if (filePath.isReadableFile())
            setupWatcherIfNeeded();
    } else {
        m_contents = contents;
    }

    if (filePath.isChildOf(base))
        setDisplayName(filePath.relativeChildPath(base).toUserOutput());
    else
        setDisplayName(filePath.toUserOutput());

    addInternalNodes();
}

void ResourceTopLevelNode::setupWatcherIfNeeded()
{
    if (m_document || !isMainThread())
        return;

    m_document = new ResourceFileWatcher(this);
    DocumentManager::addDocument(m_document);
}

ResourceTopLevelNode::~ResourceTopLevelNode()
{
    if (m_document)
        DocumentManager::removeDocument(m_document);
    delete m_document;
}

static void compressTree(FolderNode *n)
{
    if (const auto compressable = dynamic_cast<SimpleResourceFolderNode *>(n)) {
        compressable->compress();
        return;
    }
    n->forEachFolderNode([](FolderNode *c) { compressTree(c); });
}

void ResourceTopLevelNode::addInternalNodes()
{
    ResourceFile file(filePath(), m_contents);
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
            auto fn = std::make_unique<ResourceFolderNode>(file.prefix(i), file.lang(i), this);
            folderNodes.insert(prefixId, fn.get());
            addNode(std::move(fn));
        }
        auto currentPrefixNode = static_cast<ResourceFolderNode*>(folderNodes[prefixId]);

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
            for (const QString &pathElement : std::as_const(pathList)) {
                currentPathList << pathElement;
                const QString folderName = currentPathList.join(QLatin1Char('/'));
                folderId = PrefixFolderLang(prefix, folderName, lang);
                if (!folderNodes.contains(folderId)) {
                    const QString absoluteFolderName
                            = filePath().toFileInfo().absoluteDir().absoluteFilePath(
                                currentPathList.join(QLatin1Char('/')));
                    const FilePath folderPath = FilePath::fromString(absoluteFolderName);
                    std::unique_ptr<FolderNode> newNode
                            = std::make_unique<SimpleResourceFolderNode>(folderName, pathElement,
                                                                         prefix, lang, folderPath,
                                                                         this, currentPrefixNode);
                    folderNodes.insert(folderId, newNode.get());

                    PrefixFolderLang thisPrefixId = prefixId;
                    if (!parentIsPrefix)
                        thisPrefixId = PrefixFolderLang(prefix, parentFolderName, lang);
                    FolderNode *fn = folderNodes[thisPrefixId];
                    if (QTC_GUARD(fn))
                        fn->addNode(std::move(newNode));
                }
                parentIsPrefix = false;
                parentFolderName = folderName;
            }

            const QString qrcPath = QDir::cleanPath(prefixWithSlash + alias);
            fileNames.insert(fileName);
            FolderNode *fn = folderNodes[folderId];
            QTC_CHECK(fn);
            if (fn)
                fn->addNode(std::make_unique<ResourceFileNode>(FilePath::fromString(fileName),
                                                               qrcPath, displayName));
        }
    }
    compressTree(this);
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

bool ResourceTopLevelNode::addFiles(const FilePaths &filePaths, FilePaths *notAdded)
{
    return addFilesToResource(filePath(), filePaths, notAdded, "/", QString());
}

RemovedFilesFromProject ResourceTopLevelNode::removeFiles(const FilePaths &filePaths,
                                                          FilePaths *notRemoved)
{
    return parentFolderNode()->removeFiles(filePaths, notRemoved);
}

bool ResourceTopLevelNode::addPrefix(const QString &prefix, const QString &lang)
{
    ResourceFile file(filePath());
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
    ResourceFile file(filePath());
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
    ResourceFile file(filePath());
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

FolderNode::AddNewInformation ResourceTopLevelNode::addNewInformation(const FilePaths &files, Node *context) const
{
    const QString name = Tr::tr("%1 Prefix: %2").arg(filePath().fileName()).arg(QLatin1Char('/'));

    int p = getPriorityFromContextNode(this, context);
    if (p == -1 && hasPriority(files)) { // images/* and qml/js mimetypes
        p = 110;
        if (context == this)
            p = 120;
        else if (parentProjectNode() == context)
            p = 150; // steal from our project node
    }

    return AddNewInformation(name, p);
}

bool ResourceTopLevelNode::showInSimpleTree() const
{
    return true;
}

ResourceFolderNode::ResourceFolderNode(const QString &prefix, const QString &lang, ResourceTopLevelNode *parent)
    : FolderNode(parent->filePath().pathAppended(prefix)),
      // TOOD Why add existing directory doesn't work
      m_topLevelNode(parent),
      m_prefix(prefix),
      m_lang(lang)
{
}

ResourceFolderNode::~ResourceFolderNode() = default;

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
        || action == Rename // Note: only works for the filename, works akwardly for relative file paths
        || action == HidePathActions; // hides open terminal etc.
}

bool ResourceFolderNode::addFiles(const FilePaths &filePaths, FilePaths *notAdded)
{
    return addFilesToResource(m_topLevelNode->filePath(), filePaths, notAdded, m_prefix, m_lang);
}

RemovedFilesFromProject ResourceFolderNode::removeFiles(const FilePaths &filePaths,
                                                        FilePaths *notRemoved)
{
    if (notRemoved)
        *notRemoved = filePaths;
    ResourceFile file(m_topLevelNode->filePath());
    if (file.load() != IDocument::OpenResult::Success)
        return RemovedFilesFromProject::Error;
    int index = file.indexOfPrefix(m_prefix, m_lang);
    if (index == -1)
        return RemovedFilesFromProject::Error;
    for (int j = 0; j < file.fileCount(index); ++j) {
        QString fileName = file.file(index, j);
        if (!filePaths.contains(FilePath::fromString(fileName)))
            continue;
        if (notRemoved)
            notRemoved->removeOne(FilePath::fromString(fileName));
        file.removeFile(index, j);
        --j;
    }
    FileChangeBlocker changeGuard(m_topLevelNode->filePath());
    file.save();

    return RemovedFilesFromProject::Ok;
}

// QTCREATORBUG-15280
bool ResourceFolderNode::canRenameFile(const FilePath &oldFilePath, const FilePath &newFilePath)
{
    Q_UNUSED(newFilePath)

    bool fileEntryExists = false;
    ResourceFile file(m_topLevelNode->filePath());

    int index = (file.load() != IDocument::OpenResult::Success) ? -1 :file.indexOfPrefix(m_prefix, m_lang);
    if (index != -1) {
        for (int j = 0; j < file.fileCount(index); ++j) {
            if (file.file(index, j) == oldFilePath.toString()) {
                fileEntryExists = true;
                break;
            }
        }
    }

    return fileEntryExists;
}

bool ResourceFolderNode::renameFile(const FilePath &oldFilePath, const FilePath &newFilePath)
{
    ResourceFile file(m_topLevelNode->filePath());
    if (file.load() != IDocument::OpenResult::Success)
        return false;
    int index = file.indexOfPrefix(m_prefix, m_lang);
    if (index == -1)
        return false;

    for (int j = 0; j < file.fileCount(index); ++j) {
        if (file.file(index, j) == oldFilePath.toString()) {
            file.replaceFile(index, j, newFilePath.toString());
            FileChangeBlocker changeGuard(m_topLevelNode->filePath());
            file.save();
            return true;
        }
    }

    return false;
}

bool ResourceFolderNode::renamePrefix(const QString &prefix, const QString &lang)
{
    ResourceFile file(m_topLevelNode->filePath());
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

FolderNode::AddNewInformation ResourceFolderNode::addNewInformation(const FilePaths &files, Node *context) const
{
    const QString name = Tr::tr("%1 Prefix: %2").arg(m_topLevelNode->filePath().fileName())
            .arg(displayName());

    int p = getPriorityFromContextNode(this, context);
    if (p == -1 && hasPriority(files)) { // image/* and qml/js mimetypes
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

} // ResourceEditor
