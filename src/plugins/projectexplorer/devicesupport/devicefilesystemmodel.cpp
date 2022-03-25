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

#include "devicefilesystemmodel.h"

#include <utils/futuresynchronizer.h>
#include <utils/qtcassert.h>
#include <utils/runextensions.h>
#include <utils/utilsicons.h>

#include <QFutureWatcher>
#include <QIcon>
#include <QList>
#include <QScopeGuard>
#include <QSet>

using namespace Utils;

namespace ProjectExplorer {
namespace Internal {

enum class FileType {
    File,
    Dir,
//    Link,
    Other
};


class RemoteDirNode;
class RemoteFileNode
{
public:
    virtual ~RemoteFileNode() = default;

    FilePath m_filePath;
    FileType m_fileType = FileType::File;
    RemoteDirNode *m_parent = nullptr;
};

class RemoteDirNode : public RemoteFileNode
{
public:
    RemoteDirNode() { m_fileType = FileType::Dir; }
    ~RemoteDirNode() { qDeleteAll(m_children); }

    enum { Initial, Fetching, Finished } m_state = Initial;
    QList<RemoteFileNode *> m_children;
};

static RemoteFileNode *indexToFileNode(const QModelIndex &index)
{
    return static_cast<RemoteFileNode *>(index.internalPointer());
}

static RemoteDirNode *indexToDirNode(const QModelIndex &index)
{
    RemoteFileNode * const fileNode = indexToFileNode(index);
    QTC_CHECK(fileNode);
    return dynamic_cast<RemoteDirNode *>(fileNode);
}

using ResultType = QList<QPair<FilePath, FileType>>;

class DeviceFileSystemModelPrivate
{
public:
    IDevice::ConstPtr m_device;
    std::unique_ptr<RemoteDirNode> m_rootNode;
    QSet<QFutureWatcher<ResultType> *> m_watchers;
    FutureSynchronizer m_futureSynchronizer;
};

} // namespace Internal

using namespace Internal;

DeviceFileSystemModel::DeviceFileSystemModel(QObject *parent)
    : QAbstractItemModel(parent), d(new DeviceFileSystemModelPrivate)
{
    d->m_futureSynchronizer.setCancelOnWait(true);
}

DeviceFileSystemModel::~DeviceFileSystemModel()
{
    qDeleteAll(d->m_watchers);
    delete d;
}

void DeviceFileSystemModel::setDevice(const IDevice::ConstPtr &device)
{
    d->m_device = device;
}

bool DeviceFileSystemModel::canFetchMore(const QModelIndex &parent) const
{
    if (!parent.isValid()) {
        return !d->m_rootNode.get();
    }

    RemoteDirNode * const dirNode = indexToDirNode(parent);
    if (!dirNode)
        return false;
    if (dirNode->m_state == RemoteDirNode::Initial)
        return true;
    return false;
}

void DeviceFileSystemModel::fetchMore(const QModelIndex &parent)
{
    if (!parent.isValid()) {
        beginInsertRows(QModelIndex(), 0, 0);
        QTC_CHECK(!d->m_rootNode);
        d->m_rootNode.reset(new RemoteDirNode);
        d->m_rootNode->m_filePath = d->m_device->rootPath();
        endInsertRows();
        return;
    }
    RemoteDirNode * const dirNode = indexToDirNode(parent);
    if (!dirNode)
        return;
    if (dirNode->m_state != RemoteDirNode::Initial)
        return;
    collectEntries(dirNode->m_filePath, dirNode);
    dirNode->m_state = RemoteDirNode::Fetching;
}

bool DeviceFileSystemModel::hasChildren(const QModelIndex &parent) const
{
    if (!parent.isValid())
        return true;

    RemoteDirNode * const dirNode = indexToDirNode(parent);
    if (!dirNode)
        return false;
    if (dirNode->m_state == RemoteDirNode::Initial)
        return true;
    return dirNode->m_children.size();
}

int DeviceFileSystemModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return 2; // type + name
}

QVariant DeviceFileSystemModel::data(const QModelIndex &index, int role) const
{
    const RemoteFileNode * const node = indexToFileNode(index);
    QTC_ASSERT(node, return QVariant());
    if (index.column() == 0 && role == Qt::DecorationRole) {
        if (node->m_fileType == FileType::File)
            return Utils::Icons::UNKNOWN_FILE.icon();
        if (node->m_fileType == FileType::Dir)
            return Utils::Icons::DIR.icon();
        return QIcon(":/ssh/images/help.png"); // Shows a question mark.
    }
    if (index.column() == 1) {
        if (role == Qt::DisplayRole) {
            if (node->m_filePath == d->m_device->rootPath())
                return QString("/");
            return node->m_filePath.fileName();
        }
        if (role == PathRole)
            return node->m_filePath.toString();
    }
    return QVariant();
}

Qt::ItemFlags DeviceFileSystemModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;
    return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
}

QVariant DeviceFileSystemModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal)
        return QVariant();
    if (role != Qt::DisplayRole)
        return QVariant();
    if (section == 0)
        return tr("File Type");
    if (section == 1)
        return tr("File Name");
    return QVariant();
}

QModelIndex DeviceFileSystemModel::index(int row, int column, const QModelIndex &parent) const
{
    if (row < 0 || row >= rowCount(parent) || column < 0 || column >= columnCount(parent))
        return QModelIndex();
    if (!d->m_rootNode)
        return QModelIndex();
    if (!parent.isValid())
        return createIndex(row, column, d->m_rootNode.get());
    const RemoteDirNode * const parentNode = indexToDirNode(parent);
    QTC_ASSERT(parentNode, return QModelIndex());
    QTC_ASSERT(row < parentNode->m_children.count(), return QModelIndex());
    RemoteFileNode * const childNode = parentNode->m_children.at(row);
    return createIndex(row, column, childNode);
}

QModelIndex DeviceFileSystemModel::parent(const QModelIndex &child) const
{
    if (!child.isValid()) // Don't assert on this, since the model tester tries it.
        return QModelIndex();

    const RemoteFileNode * const childNode = indexToFileNode(child);
    QTC_ASSERT(childNode, return QModelIndex());
    if (childNode == d->m_rootNode.get())
        return QModelIndex();
    RemoteDirNode * const parentNode = childNode->m_parent;
    if (parentNode == d->m_rootNode.get())
        return createIndex(0, 0, d->m_rootNode.get());
    const RemoteDirNode * const grandParentNode = parentNode->m_parent;
    QTC_ASSERT(grandParentNode, return QModelIndex());
    return createIndex(grandParentNode->m_children.indexOf(parentNode), 0, parentNode);
}

static FileType fileType(const FilePath &path)
{
    if (path.isDir())
        return FileType::Dir;
    if (path.isFile())
        return FileType::File;
    return FileType::Other;
}

static void dirEntries(QFutureInterface<ResultType> &futureInterface, const FilePath &dir)
{
    const QList<FilePath> entries = dir.dirEntries(QDir::NoFilter);
    ResultType result;
    for (const FilePath &entry : entries) {
        if (futureInterface.isCanceled())
            return;
        result.append(qMakePair(entry, fileType(entry)));
    }
    futureInterface.reportResult(result);
}

int DeviceFileSystemModel::rowCount(const QModelIndex &parent) const
{
    if (!d->m_rootNode)
        return 0;
    if (!parent.isValid())
        return 1;
    if (parent.column() != 0)
        return 0;
    RemoteDirNode * const dirNode = indexToDirNode(parent);
    if (!dirNode)
        return 0;
    return dirNode->m_children.count();
}

void DeviceFileSystemModel::collectEntries(const FilePath &filePath, RemoteDirNode *parentNode)
{
    // Destructor of this will delete working watchers, as they are children of this.
    QFutureWatcher<ResultType> *watcher = new QFutureWatcher<ResultType>(this);
    auto future = runAsync(dirEntries, filePath);
    d->m_futureSynchronizer.addFuture(future);
    connect(watcher, &QFutureWatcher<ResultType>::finished, this, [this, watcher, parentNode] {
        auto cleanup = qScopeGuard([watcher, this] {
            d->m_watchers.remove(watcher);
            watcher->deleteLater();
        });

        QTC_ASSERT(parentNode->m_state == RemoteDirNode::Fetching, return);
        parentNode->m_state = RemoteDirNode::Finished;

        const ResultType entries = watcher->result();
        if (entries.isEmpty())
            return;

        const int row = parentNode->m_parent
                ? parentNode->m_parent->m_children.indexOf(parentNode) : 0;
        const QModelIndex parentIndex = createIndex(row, 0, parentNode);
        beginInsertRows(parentIndex, 0, entries.count() - 1);

        for (const QPair<FilePath, FileType> &entry : entries) {
            RemoteFileNode *childNode = nullptr;
            if (entry.second == FileType::Dir)
                childNode = new RemoteDirNode;
            else
                childNode = new RemoteFileNode;
            childNode->m_filePath = entry.first;
            childNode->m_fileType = entry.second;
            childNode->m_parent = parentNode;
            parentNode->m_children.append(childNode);
        }
        endInsertRows();
    });
    d->m_watchers.insert(watcher);
    watcher->setFuture(future);
}

} // namespace ProjectExplorer
