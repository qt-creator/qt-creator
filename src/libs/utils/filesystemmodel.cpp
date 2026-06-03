// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "filesystemmodel.h"

#include "filepathinfo.h"
#include "fsengine/fileiconprovider.h"
#include "utiltypes.h"

#include <QDir>
#include <QDirIterator>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QHash>
#include <QIcon>
#include <QImageReader>
#include <QLocale>
#include <QPixmap>
#include <QQueue>
#include <QSet>
#include <QThread>

#include <map>

using namespace Qt::StringLiterals;

namespace Utils {

// Returns an icon for the file. For local files of a supported image format the
// image content itself is loaded and scaled to thumbnail size, so the user gets
// a visual preview rather than a generic type icon.
static QIcon iconForPath(const FilePath &path)
{
    static const QSet<QString> imageFormats = [] {
        QSet<QString> formats;
        for (const QByteArray &fmt : QImageReader::supportedImageFormats())
            formats.insert(QString::fromLatin1(fmt).toLower());
        return formats;
    }();

    if (imageFormats.contains(path.suffix().toLower())) {
        static constexpr QSize thumbnailSize{128, 128};
        QImageReader reader(path.toFSPathString());
        reader.setAutoTransform(true);
        const QSize original = reader.size();
        if (original.isValid())
            reader.setScaledSize(original.scaled(thumbnailSize, Qt::KeepAspectRatio));
        QImage image = reader.read();
        if (!image.isNull())
            return QIcon(QPixmap::fromImage(std::move(image)));
    }

    return path.icon();
}

// ===== Worker — lives in background thread =====
//
// Handles all filesystem access: directory listing (with FilePathInfo) and
// icon resolution (which may involve file content reads on some platforms).
//
// Icons are processed one at a time from a queue so that fetchChildren slots
// can interleave between icon fetches instead of being blocked by a large batch.

class FileSystemWorker : public QObject
{
    Q_OBJECT
public:
    explicit FileSystemWorker(QObject *parent = nullptr)
        : QObject(parent)
    {}

public slots:
    void fetchChildren(const Utils::FilePath &dirPath, quint64 generation)
    {
        FilePaths children;
        QList<FilePathInfo> infos;

        dirPath.iterateDirectory(
            [&](const FilePath &path, const FilePathInfo &info) {
                children.append(path);
                infos.append(info);
                return IterationPolicy::Continue;
            },
            FileFilter(
                {},
                DirFilterFlag::AllEntries | DirFilterFlag::NoDotAndDotDot | DirFilterFlag::Hidden));

        emit childrenFetched(dirPath, children, infos, generation);
    }

    void queueIcons(const Utils::FilePaths &paths)
    {
        for (const FilePath &path : paths)
            m_iconQueue.enqueue(path);
        scheduleNextIcon();
    }

    void clearIconQueue()
    {
        m_iconQueue.clear();
    }

signals:
    void childrenFetched(const Utils::FilePath &dirPath,
                         const Utils::FilePaths &children,
                         const QList<Utils::FilePathInfo> &infos,
                         quint64 generation);
    void iconFetched(const Utils::FilePath &path, const QIcon &icon);

private slots:
    void processNextIcon()
    {
        QElapsedTimer elapsed;
        elapsed.start();
        while (!m_iconQueue.isEmpty() && elapsed.elapsed() < 500) {
            const FilePath path = m_iconQueue.dequeue();
            emit iconFetched(path, iconForPath(path));
        }
        scheduleNextIcon();
    }

private:
    void scheduleNextIcon()
    {
        if (!m_iconQueue.isEmpty())
            QMetaObject::invokeMethod(this,
                                      &FileSystemWorker::processNextIcon,
                                      Qt::QueuedConnection);
    }

    QQueue<FilePath> m_iconQueue;
};

// ===== Node =====

struct FileSystemNode
{
    explicit FileSystemNode(const FilePath &path, FileSystemNode *parentNode = nullptr, int row = 0)
        : m_path(path)
        , m_parentNode(parentNode)
        , m_row(row)
    {}

    ~FileSystemNode() { qDeleteAll(m_children); }

    int row() const { return m_row; }

    bool isDir() const { return m_info.fileFlags & FilePathInfo::DirectoryType; }

    FilePath m_path;
    FileSystemNode *m_parentNode = nullptr;
    QList<FileSystemNode *> m_children;
    FilePathInfo m_info;
    QIcon m_icon;
    int m_row;

    enum class State { NotFetched, Fetching, Fetched };
    State m_state = State::NotFetched;
};

// ===== Private =====

class FileSystemModelPrivate : public QObject
{
    Q_OBJECT
public:
    explicit FileSystemModelPrivate(FileSystemModel *q);
    ~FileSystemModelPrivate() override;

    void setRootPath(const FilePath &path);
    void rebuildTree();
    FileSystemNode *nodeForIndex(const QModelIndex &index) const;
    QModelIndex indexForNode(FileSystemNode *node, int column = 0) const;
    void triggerFetch(FileSystemNode *node);
    void removeNodeFromIndex(FileSystemNode *node);
    void applyChildrenDiff(
        FileSystemNode *node,
        const QModelIndex &parentIndex,
        const FilePaths &newPaths,
        const QList<FilePathInfo> &newInfos);

signals:
    void requestFetchChildren(const Utils::FilePath &path, quint64 generation);
    void requestQueueIcons(const Utils::FilePaths &paths);
    void requestClearIconQueue();

private slots:
    void onChildrenFetched(const Utils::FilePath &dirPath,
                           const Utils::FilePaths &children,
                           const QList<Utils::FilePathInfo> &infos,
                           quint64 generation);
    void onIconFetched(const Utils::FilePath &path, const QIcon &icon);
    void flushIconUpdates();
    void onDirectoryChanged(const Utils::FilePath &path);

public:
    FileSystemModel *m_q;
    FileSystemNode *m_root = nullptr;
    FilePath m_rootPath;
    quint64 m_generation = 0;
    QThread m_workerThread;
    FileSystemWorker *m_worker = nullptr;
    std::map<FilePath, std::unique_ptr<FilePathWatcher>> m_dirWatchers;
    QHash<FilePath, FileSystemNode *> m_nodeIndex;

private:
    QList<FileSystemNode *> m_pendingIconNodes;
    bool m_iconFlushScheduled = false;
};

FileSystemModelPrivate::FileSystemModelPrivate(FileSystemModel *q)
    : QObject(q)
    , m_q(q)
{
    m_worker = new FileSystemWorker;
    m_worker->moveToThread(&m_workerThread);

    connect(this,
            &FileSystemModelPrivate::requestFetchChildren,
            m_worker,
            &FileSystemWorker::fetchChildren);
    connect(this,
            &FileSystemModelPrivate::requestQueueIcons,
            m_worker,
            &FileSystemWorker::queueIcons);
    connect(this,
            &FileSystemModelPrivate::requestClearIconQueue,
            m_worker,
            &FileSystemWorker::clearIconQueue);
    connect(m_worker,
            &FileSystemWorker::childrenFetched,
            this,
            &FileSystemModelPrivate::onChildrenFetched);
    connect(m_worker,
            &FileSystemWorker::iconFetched,
            this,
            &FileSystemModelPrivate::onIconFetched);

    m_workerThread.start();

    m_root = new FileSystemNode({});
    m_root->m_info.fileFlags = FilePathInfo::DirectoryType;
    m_root->m_info.fileFlags |= FilePathInfo::ExistsFlag;
    m_root->m_state = FileSystemNode::State::Fetched;
}

FileSystemModelPrivate::~FileSystemModelPrivate()
{
    m_workerThread.quit();
    m_workerThread.wait();
    delete m_worker;
    delete m_root;
}

void FileSystemModelPrivate::rebuildTree()
{
    ++m_generation;
    emit requestClearIconQueue();

    m_q->beginResetModel();
    m_dirWatchers.clear();
    qDeleteAll(m_root->m_children);
    m_root->m_children.clear();
    m_nodeIndex.clear();
    m_pendingIconNodes.clear();
    m_iconFlushScheduled = false;

    if (!m_rootPath.isEmpty()) {
        auto *node = new FileSystemNode(m_rootPath, m_root, 0);
        node->m_info.fileFlags = FilePathInfo::DirectoryType;
        node->m_info.fileFlags |= FilePathInfo::ExistsFlag;
        m_root->m_children.append(node);
        m_nodeIndex[m_rootPath] = node;
    } else {
        const QList<QFileInfo> drives = QDir::drives();
        for (int i = 0; i < drives.size(); ++i) {
            const FilePath drivePath = FilePath::fromString(drives[i].absoluteFilePath());
            auto *node = new FileSystemNode(drivePath, m_root, i);
            node->m_info.fileFlags = FilePathInfo::DirectoryType;
            node->m_info.fileFlags |= FilePathInfo::ExistsFlag;
            m_root->m_children.append(node);
            m_nodeIndex[drivePath] = node;
        }
    }

    m_q->endResetModel();
}

void FileSystemModelPrivate::setRootPath(const FilePath &path)
{
    if (m_rootPath == path)
        return;

    m_rootPath = path;
    rebuildTree();

    if (!path.isEmpty())
        emit m_q->rootPathChanged(path);
}

FileSystemNode *FileSystemModelPrivate::nodeForIndex(const QModelIndex &index) const
{
    if (!index.isValid())
        return m_root;
    return static_cast<FileSystemNode *>(index.internalPointer());
}

QModelIndex FileSystemModelPrivate::indexForNode(FileSystemNode *node, int column) const
{
    if (!node || node == m_root)
        return {};
    return m_q->createIndex(node->row(), column, node);
}

void FileSystemModelPrivate::triggerFetch(FileSystemNode *node)
{
    if (node->m_state != FileSystemNode::State::NotFetched)
        return;
    node->m_state = FileSystemNode::State::Fetching;
    emit requestFetchChildren(node->m_path, m_generation);
}

void FileSystemModelPrivate::onChildrenFetched(const Utils::FilePath &dirPath,
                                               const Utils::FilePaths &children,
                                               const QList<Utils::FilePathInfo> &infos,
                                               quint64 generation)
{
    if (generation != m_generation)
        return;

    FileSystemNode *node = m_nodeIndex.value(dirPath, nullptr);
    if (!node || node->m_state != FileSystemNode::State::Fetching)
        return;

    const QModelIndex parentIndex = indexForNode(node);

    if (node->m_children.isEmpty()) {
        if (!children.isEmpty()) {
            m_q->beginInsertRows(parentIndex, 0, static_cast<int>(children.size()) - 1);
            for (int i = 0; i < children.size(); ++i) {
                auto *child = new FileSystemNode(children[i], node, i);
                child->m_info = infos[i];
                node->m_children.append(child);
                m_nodeIndex[children[i]] = child;
            }
            m_q->endInsertRows();
            emit requestQueueIcons(children);
        }
    } else {
        applyChildrenDiff(node, parentIndex, children, infos);
    }

    if (!m_dirWatchers.contains(dirPath)) {
        auto result = dirPath.watch();
        if (result) {
            connect(
                result->get(),
                &FilePathWatcher::pathChanged,
                this,
                &FileSystemModelPrivate::onDirectoryChanged);
            m_dirWatchers.emplace(dirPath, std::move(*result));
        }
    }

    node->m_state = FileSystemNode::State::Fetched;
    emit m_q->directoryLoaded(dirPath);
}

void FileSystemModelPrivate::onDirectoryChanged(const Utils::FilePath &path)
{
    FileSystemNode *node = m_nodeIndex.value(path, nullptr);
    if (!node || node->m_state == FileSystemNode::State::Fetching)
        return;
    node->m_state = FileSystemNode::State::Fetching;
    emit requestFetchChildren(path, m_generation);
}

void FileSystemModelPrivate::removeNodeFromIndex(FileSystemNode *node)
{
    m_nodeIndex.remove(node->m_path);
    m_dirWatchers.erase(node->m_path);
    m_pendingIconNodes.removeAll(node);
    for (FileSystemNode *child : std::as_const(node->m_children))
        removeNodeFromIndex(child);
}

void FileSystemModelPrivate::applyChildrenDiff(
    FileSystemNode *node,
    const QModelIndex &parentIndex,
    const FilePaths &newPaths,
    const QList<FilePathInfo> &newInfos)
{
    QHash<FilePath, int> newPathIndex;
    newPathIndex.reserve(newPaths.size());
    for (int i = 0; i < newPaths.size(); ++i)
        newPathIndex[newPaths[i]] = i;

    // Collect rows to remove (back-to-front to keep indices stable)
    QList<int> toRemove;
    for (int i = 0; i < node->m_children.size(); ++i) {
        if (!newPathIndex.contains(node->m_children[i]->m_path))
            toRemove.append(i);
    }
    for (int i = toRemove.size() - 1; i >= 0; --i) {
        const int row = toRemove[i];
        m_q->beginRemoveRows(parentIndex, row, row);
        FileSystemNode *child = node->m_children.takeAt(row);
        removeNodeFromIndex(child);
        delete child;
        m_q->endRemoveRows();
    }

    // Re-number after removals
    for (int i = 0; i < node->m_children.size(); ++i)
        node->m_children[i]->m_row = i;

    // Update info for surviving children; collect changed rows for dataChanged
    int minChanged = INT_MAX, maxChanged = INT_MIN;
    for (int i = 0; i < node->m_children.size(); ++i) {
        FileSystemNode *child = node->m_children[i];
        const auto it = newPathIndex.constFind(child->m_path);
        if (it != newPathIndex.constEnd()) {
            const FilePathInfo &newInfo = newInfos[it.value()];
            if (child->m_info.fileSize != newInfo.fileSize
                || child->m_info.lastModified != newInfo.lastModified) {
                child->m_info = newInfo;
                minChanged = qMin(minChanged, i);
                maxChanged = qMax(maxChanged, i);
            }
        }
    }
    if (minChanged <= maxChanged) {
        const QModelIndex tl = m_q->createIndex(
            minChanged, FileSystemModel::SizeColumn, node->m_children.at(minChanged));
        const QModelIndex br = m_q->createIndex(
            maxChanged, FileSystemModel::DateModifiedColumn, node->m_children.at(maxChanged));
        emit m_q->dataChanged(tl, br);
    }

    // Insert new children not in old set
    QSet<FilePath> existing;
    existing.reserve(node->m_children.size());
    for (FileSystemNode *child : std::as_const(node->m_children))
        existing.insert(child->m_path);

    FilePaths toAdd;
    QList<FilePathInfo> toAddInfos;
    for (int i = 0; i < newPaths.size(); ++i) {
        if (!existing.contains(newPaths[i])) {
            toAdd.append(newPaths[i]);
            toAddInfos.append(newInfos[i]);
        }
    }
    if (!toAdd.isEmpty()) {
        const int firstRow = node->m_children.size();
        m_q->beginInsertRows(parentIndex, firstRow, firstRow + toAdd.size() - 1);
        for (int i = 0; i < toAdd.size(); ++i) {
            auto *child = new FileSystemNode(toAdd[i], node, firstRow + i);
            child->m_info = toAddInfos[i];
            node->m_children.append(child);
            m_nodeIndex[toAdd[i]] = child;
        }
        m_q->endInsertRows();
        emit requestQueueIcons(toAdd);
    }
}

void FileSystemModelPrivate::onIconFetched(const Utils::FilePath &path, const QIcon &icon)
{
    FileSystemNode *node = m_nodeIndex.value(path, nullptr);
    if (!node)
        return;
    node->m_icon = icon;
    m_pendingIconNodes.append(node);

    if (!m_iconFlushScheduled) {
        m_iconFlushScheduled = true;
        QMetaObject::invokeMethod(this,
                                  &FileSystemModelPrivate::flushIconUpdates,
                                  Qt::QueuedConnection);
    }
}

void FileSystemModelPrivate::flushIconUpdates()
{
    m_iconFlushScheduled = false;

    // Group dirty rows by parent so we emit one ranged dataChanged per directory.
    QHash<FileSystemNode *, std::pair<int, int>> dirtyRanges;
    for (FileSystemNode *node : std::as_const(m_pendingIconNodes)) {
        auto it = dirtyRanges.find(node->m_parentNode);
        if (it == dirtyRanges.end())
            dirtyRanges.insert(node->m_parentNode, {node->m_row, node->m_row});
        else
            *it = {qMin(it->first, node->m_row), qMax(it->second, node->m_row)};
    }
    m_pendingIconNodes.clear();

    for (auto it = dirtyRanges.cbegin(); it != dirtyRanges.cend(); ++it) {
        FileSystemNode *parent = it.key();
        const auto [minRow, maxRow] = it.value();
        const QModelIndex topLeft = m_q->createIndex(minRow, FileSystemModel::NameColumn,
                                                     parent->m_children.at(minRow));
        const QModelIndex bottomRight = m_q->createIndex(maxRow, FileSystemModel::NameColumn,
                                                         parent->m_children.at(maxRow));
        emit m_q->dataChanged(topLeft, bottomRight, {Qt::DecorationRole});
    }
}

// ===== FileSystemProxyModel =====

FileSystemProxyModel::FileSystemProxyModel(QObject *parent)
    : QSortFilterProxyModel(parent)
{}

void FileSystemProxyModel::setShowHiddenFiles(bool show)
{
    if (m_showHiddenFiles == show)
        return;
    m_showHiddenFiles = show;
    invalidateFilter();
}

bool FileSystemProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    const QModelIndex sourceIndex = sourceModel()->index(sourceRow, 0, sourceParent);

    if (!m_showHiddenFiles) {
        const auto flags = sourceIndex.data(FileSystemModel::FileFlagsRole)
                               .value<FilePathInfo::FileFlags>();
        if (flags & FilePathInfo::HiddenFlag)
            return false;
        if (sourceIndex.data(FileSystemModel::FileNameRole).toString().startsWith('.'))
            return false;
    }

    if (const auto *fsModel = qobject_cast<FileSystemModel *>(sourceModel())) {
        if (fsModel->rootPath().isEmpty()) {
            QModelIndex idx = sourceIndex;
            while (idx.isValid()) {
                if (idx.data().toString() == FilePath::specialRootName())
                    return false;
                idx = idx.parent();
            }
        }
    }

    return QSortFilterProxyModel::filterAcceptsRow(sourceRow, sourceParent);
}

bool FileSystemProxyModel::lessThan(const QModelIndex &sourceLeft,
                                    const QModelIndex &sourceRight) const
{
    const QAbstractItemModel *src = sourceModel();
    if (sortRole() == FileSystemModel::FileSortRole) {
        return src->data(sourceLeft, FileSystemModel::FileSortRole).toString()
               < src->data(sourceRight, FileSystemModel::FileSortRole).toString();
    }
    return FilePath::fromString(src->data(sourceLeft, FileSystemModel::FileNameRole).toString())
           < FilePath::fromString(src->data(sourceRight, FileSystemModel::FileNameRole).toString());
}

// ===== FileSystemModel =====

FileSystemModel::FileSystemModel(QObject *parent)
    : QAbstractItemModel(parent)
    , d(std::make_unique<FileSystemModelPrivate>(this))
{}

FileSystemModel::~FileSystemModel() = default;

FilePath FileSystemModel::rootPath() const
{
    return d->m_rootPath;
}

void FileSystemModel::setRootPath(const FilePath &rootPath)
{
    d->setRootPath(rootPath);
}

FilePath FileSystemModel::filePath(const QModelIndex &index) const
{
    FileSystemNode *node = d->nodeForIndex(index);
    if (node == d->m_root)
        return {};
    return node->m_path;
}

bool FileSystemModel::isDir(const QModelIndex &index) const
{
    FileSystemNode *node = d->nodeForIndex(index);
    return node && node->isDir();
}

QModelIndex FileSystemModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent))
        return {};

    FileSystemNode *parentNode = d->nodeForIndex(parent);
    if (row >= parentNode->m_children.size())
        return {};

    return createIndex(row, column, parentNode->m_children.at(row));
}

QModelIndex FileSystemModel::parent(const QModelIndex &child) const
{
    if (!child.isValid())
        return {};

    FileSystemNode *node = static_cast<FileSystemNode *>(child.internalPointer());
    FileSystemNode *parentNode = node->m_parentNode;

    if (!parentNode || parentNode == d->m_root)
        return {};

    return createIndex(parentNode->row(), 0, parentNode);
}

int FileSystemModel::rowCount(const QModelIndex &parent) const
{
    if (parent.column() > 0)
        return 0;
    return d->nodeForIndex(parent)->m_children.size();
}

int FileSystemModel::columnCount(const QModelIndex &) const
{
    return NumColumns;
}

bool FileSystemModel::hasChildren(const QModelIndex &parent) const
{
    if (parent.column() > 0)
        return false;

    FileSystemNode *node = d->nodeForIndex(parent);

    if (node->m_state == FileSystemNode::State::Fetched)
        return !node->m_children.isEmpty();

    return node->isDir();
}

bool FileSystemModel::canFetchMore(const QModelIndex &parent) const
{
    FileSystemNode *node = d->nodeForIndex(parent);
    return node->isDir() && node->m_state == FileSystemNode::State::NotFetched;
}

void FileSystemModel::fetchMore(const QModelIndex &parent)
{
    d->triggerFetch(d->nodeForIndex(parent));
}

Qt::ItemFlags FileSystemModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QVariant FileSystemModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return {};

    FileSystemNode *node = static_cast<FileSystemNode *>(index.internalPointer());

    switch (role) {
    case Qt::DisplayRole:
        switch (index.column()) {
        case NameColumn:
            if (node->m_path.isRootPath())
                return node->m_path.displayName();
            return node->m_path.fileName();
        case SizeColumn:
            if (!node->isDir())
                return QLocale().formattedDataSize(node->m_info.fileSize);
            return {};
        case TypeColumn:
            if (node->isDir())
                return tr("Directory");
            if (const QString ext = node->m_path.suffix(); !ext.isEmpty())
                return u"%1 File"_s.arg(ext.toUpper());
            return tr("File");
        case DateModifiedColumn:
            if (node->m_info.lastModified.isValid())
                return QLocale().toString(node->m_info.lastModified, QLocale::ShortFormat);
            return {};
        }
        break;

    case Qt::DecorationRole:
        if (index.column() == NameColumn) {
            if (!node->m_icon.isNull())
                return node->m_icon;
            return node->isDir() ? FileIconProvider::icon(QFileIconProvider::Folder)
                                 : FileIconProvider::icon(QFileIconProvider::File);
        }
        break;

    case FilePathRole:
        return QVariant::fromValue(node->m_path);

    case FileNameRole:
        return node->m_path.fileName();

    case FileFlagsRole:
        return QVariant::fromValue(node->m_info.fileFlags);

    case FileSortRole:
        // Directories sort before files, then alphabetically case-insensitive
        return QVariant(QString(node->isDir() ? QChar('0') : QChar('1'))
                        + node->m_path.fileName().toLower());
    }

    return {};
}

QVariant FileSystemModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
        return {};

    switch (section) {
    case NameColumn:         return tr("Name");
    case SizeColumn:         return tr("Size");
    case TypeColumn:         return tr("Type");
    case DateModifiedColumn: return tr("Date Modified");
    }
    return {};
}


QModelIndex FileSystemModel::index(const FilePath &path) const
{
    auto it = d->m_nodeIndex.constFind(path);
    if (it == d->m_nodeIndex.constEnd())
        return {};
    return d->indexForNode(it.value());
}

bool FileSystemModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (role != Qt::EditRole || !index.isValid())
        return false;
    FileSystemNode *node = static_cast<FileSystemNode *>(index.internalPointer());
    const QString newName = value.toString();
    if (newName.isEmpty() || newName == node->m_path.fileName())
        return false;
    const FilePath newPath = node->m_path.parentDir().pathAppended(newName);
    if (!node->m_path.renameFile(newPath))
        return false;
    d->m_nodeIndex.remove(node->m_path);
    node->m_path = newPath;
    d->m_nodeIndex[newPath] = node;
    emit dataChanged(index, index);
    return true;
}

QModelIndex FileSystemModel::mkdir(const QModelIndex &parent, const QString &name)
{
    FileSystemNode *parentNode = d->nodeForIndex(parent);
    if (!parentNode || !parentNode->isDir())
        return {};
    const FilePath newPath = parentNode->m_path.pathAppended(name);
    if (!newPath.createDir())
        return {};
    const int row = parentNode->m_children.size();
    beginInsertRows(parent, row, row);
    auto *newNode = new FileSystemNode(newPath, parentNode, row);
    newNode->m_info.fileFlags = FilePathInfo::DirectoryType;
    newNode->m_info.fileFlags |= FilePathInfo::ExistsFlag;
    newNode->m_state = FileSystemNode::State::NotFetched;
    parentNode->m_children.append(newNode);
    d->m_nodeIndex[newPath] = newNode;
    endInsertRows();
    return d->indexForNode(newNode);
}

} // namespace Utils

#include "filesystemmodel.moc"
