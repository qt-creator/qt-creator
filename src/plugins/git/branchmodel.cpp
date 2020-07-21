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

#include "branchmodel.h"
#include "gitclient.h"
#include "gitconstants.h"

#include <vcsbase/vcsoutputwindow.h>
#include <vcsbase/vcscommand.h>

#include <utils/filesystemwatcher.h>
#include <utils/qtcassert.h>
#include <utils/stringutils.h>

#include <QDateTime>
#include <QFont>

#include <set>

using namespace VcsBase;

namespace Git {
namespace Internal {

enum RootNodes {
    LocalBranches = 0,
    RemoteBranches = 1,
    Tags = 2
};

enum Columns {
    ColumnBranch = 0,
    ColumnDateTime = 1,
    ColumnCount
};

// --------------------------------------------------------------------------
// BranchNode:
// --------------------------------------------------------------------------

class BranchNode : public QObject
{
public:
    BranchNode() :
        name("<ROOT>")
    { }

    BranchNode(const QString &n, const QString &s = QString(), const QString &t = QString(),
               const QDateTime &dt = QDateTime()) :
        name(n), sha(s), tracking(t), dateTime(dt)
    { }

    ~BranchNode()
    {
        while (!children.isEmpty())
            delete children.first();
        if (parent)
            parent->children.removeAll(this);
    }

    BranchNode *rootNode() const
    {
        return parent ? parent->rootNode() : const_cast<BranchNode *>(this);
    }

    int count() const
    {
        return children.count();
    }

    bool isLeaf() const
    {
        return children.isEmpty() && parent && parent->parent;
    }

    bool childOf(BranchNode *node) const
    {
        if (this == node)
            return true;
        return parent ? parent->childOf(node) : false;
    }

    bool childOfRoot(RootNodes root) const
    {
        BranchNode *rn = rootNode();
        if (rn->isLeaf())
            return false;
        if (root >= rn->children.count())
            return false;
        return childOf(rn->children.at(root));
    }

    bool isTag() const
    {
        return childOfRoot(Tags);
    }

    bool isLocal() const
    {
        return childOfRoot(LocalBranches);
    }

    BranchNode *childOfName(const QString &name) const
    {
        for (int i = 0; i < children.count(); ++i) {
            if (children.at(i)->name == name)
                return children.at(i);
        }
        return nullptr;
    }

    QStringList fullName(bool includePrefix = false) const
    {
        QStringList fn;
        QList<const BranchNode *> nodes;
        const BranchNode *current = this;
        while (current->parent) {
            nodes.prepend(current);
            current = current->parent;
        }

        if (includePrefix)
            fn.append(nodes.first()->sha);
        nodes.removeFirst();

        for (const BranchNode *n : qAsConst(nodes))
            fn.append(n->name);

        return fn;
    }

    QString fullRef(bool includePrefix = false) const
    {
        return fullName(includePrefix).join('/');
    }

    void insert(const QStringList &path, BranchNode *n)
    {
        BranchNode *current = this;
        for (int i = 0; i < path.count(); ++i) {
            BranchNode *c = current->childOfName(path.at(i));
            if (c)
                current = c;
            else
                current = current->append(new BranchNode(path.at(i)));
        }
        current->append(n);
    }

    BranchNode *append(BranchNode *n)
    {
        n->parent = this;
        children.append(n);
        return n;
    }

    BranchNode *prepend(BranchNode *n)
    {
        n->parent = this;
        children.prepend(n);
        return n;
    }

    QStringList childrenNames() const
    {
        if (!children.isEmpty()) {
            QStringList names;
            for (BranchNode *n : children) {
                names.append(n->childrenNames());
            }
            return names;
        }
        return {fullRef()};
    }

    int rowOf(BranchNode *node)
    {
        return children.indexOf(node);
    }

    void setUpstreamStatus(UpstreamStatus newStatus)
    {
        status = newStatus;
    }

    BranchNode *parent = nullptr;
    QList<BranchNode *> children;

    QString name;
    QString sha;
    QString tracking;
    QDateTime dateTime;
    UpstreamStatus status;
    mutable QString toolTip;
};

class BranchModel::Private
{
public:
    explicit Private(BranchModel *q, GitClient *client) :
        q(q),
        client(client),
        rootNode(new BranchNode)
    {
    }

    Private(const Private &) = delete;
    Private &operator=(const Private &) = delete;

    ~Private()
    {
        delete rootNode;
    }

    bool hasTags() const { return rootNode->children.count() > Tags; }
    void parseOutputLine(const QString &line, bool force = false);
    void flushOldEntries();

    BranchModel *q;
    GitClient *client;
    QString workingDirectory;
    BranchNode *rootNode;
    BranchNode *currentBranch = nullptr;
    BranchNode *headNode = nullptr;
    QString currentSha;
    QDateTime currentDateTime;
    QStringList obsoleteLocalBranches;
    Utils::FileSystemWatcher fsWatcher;
    bool oldBranchesIncluded = false;

    struct OldEntry
    {
        QString line;
        QDateTime dateTime;
        bool operator<(const OldEntry &other) const { return dateTime < other.dateTime; }
    };

    BranchNode *currentRoot = nullptr;
    QString currentRemote;
    std::set<OldEntry> oldEntries;
};

// --------------------------------------------------------------------------
// BranchModel:
// --------------------------------------------------------------------------

BranchModel::BranchModel(GitClient *client, QObject *parent) :
    QAbstractItemModel(parent),
    d(new Private(this, client))
{
    QTC_CHECK(d->client);

    // Abuse the sha field for ref prefix
    d->rootNode->append(new BranchNode(tr("Local Branches"), "refs/heads"));
    d->rootNode->append(new BranchNode(tr("Remote Branches"), "refs/remotes"));
    connect(&d->fsWatcher, &Utils::FileSystemWatcher::fileChanged, this, [this] {
        QString errorMessage;
        refresh(d->workingDirectory, &errorMessage);
    });
}

BranchModel::~BranchModel()
{
    delete d;
}

QModelIndex BranchModel::index(int row, int column, const QModelIndex &parentIdx) const
{
    if (column > 1)
        return QModelIndex();
    BranchNode *parentNode = indexToNode(parentIdx);

    if (row >= parentNode->count())
        return QModelIndex();
    return nodeToIndex(parentNode->children.at(row), column);
}

QModelIndex BranchModel::parent(const QModelIndex &index) const
{
    if (!index.isValid())
        return QModelIndex();

    BranchNode *node = indexToNode(index);
    if (node->parent == d->rootNode)
        return QModelIndex();
    return nodeToIndex(node->parent, ColumnBranch);
}

int BranchModel::rowCount(const QModelIndex &parentIdx) const
{
    if (parentIdx.column() > 0)
        return 0;

    return indexToNode(parentIdx)->count();
}

int BranchModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return ColumnCount;
}

QVariant BranchModel::data(const QModelIndex &index, int role) const
{
    const QChar arrowUp(0x2191);
    const QChar arrowDown(0x2193);

    BranchNode *node = indexToNode(index);
    if (!node)
        return QVariant();

    switch (role) {
    case Qt::DisplayRole: {
        QString res;
        switch (index.column()) {
        case ColumnBranch: {
            res = node->name;
            if (!node->tracking.isEmpty()) {
                res += ' ' + arrowUp + QString::number(node->status.ahead);
                res += ' ' + arrowDown + QString::number(node->status.behind);
                res += " [" + node->tracking + ']';
            }
            break;
        }
        case ColumnDateTime:
            if (node->isLeaf() && node->dateTime.isValid())
                res = node->dateTime.toString("yyyy-MM-dd HH:mm");
            break;
        }
        return res;
    }
    case Qt::EditRole:
        return index.column() == 0 ? node->fullRef() : QVariant();
    case Qt::ToolTipRole:
        if (!node->isLeaf())
            return QVariant();
        if (node->toolTip.isEmpty())
            node->toolTip = toolTip(node->sha);
        return node->toolTip;
    case Qt::FontRole:
    {
        QFont font;
        if (!node->isLeaf()) {
            font.setBold(true);
        } else if (node == d->currentBranch) {
            font.setBold(true);
            font.setUnderline(true);
        }
        return font;
    }
    default:
        return QVariant();
    }
}

bool BranchModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (index.column() != ColumnBranch || role != Qt::EditRole)
        return false;
    BranchNode *node = indexToNode(index);
    if (!node)
        return false;

    const QString newName = value.toString();
    if (newName.isEmpty())
        return false;

    const QString oldName = node->fullRef();
    if (oldName == newName)
        return false;

    renameBranch(oldName, newName);
    return true;
}

Qt::ItemFlags BranchModel::flags(const QModelIndex &index) const
{
    BranchNode *node = indexToNode(index);
    if (!node)
        return Qt::NoItemFlags;
    Qt::ItemFlags res = Qt::ItemIsSelectable | Qt::ItemIsEnabled;
    if (node != d->headNode && node->isLeaf() && node->isLocal() && index.column() == ColumnBranch)
        res |= Qt::ItemIsEditable;
    return res;
}

void BranchModel::clear()
{
    for (BranchNode *root : qAsConst(d->rootNode->children)) {
        while (root->count())
            delete root->children.takeLast();
    }
    if (d->hasTags())
        d->rootNode->children.takeLast();

    d->currentSha.clear();
    d->currentDateTime = QDateTime();
    d->currentBranch = nullptr;
    d->headNode = nullptr;
    d->obsoleteLocalBranches.clear();
}

bool BranchModel::refresh(const QString &workingDirectory, QString *errorMessage)
{
    beginResetModel();
    clear();
    if (workingDirectory.isEmpty()) {
        endResetModel();
        return true;
    }

    d->currentSha = d->client->synchronousTopRevision(workingDirectory, &d->currentDateTime);
    const QStringList args = {"--format=%(objectname)\t%(refname)\t%(upstream:short)\t"
                              "%(*objectname)\t%(committerdate:raw)\t%(*committerdate:raw)"};
    QString output;
    if (!d->client->synchronousForEachRefCmd(workingDirectory, args, &output, errorMessage)) {
        endResetModel();
        return false;
    }

    if (d->workingDirectory != workingDirectory) {
        d->workingDirectory = workingDirectory;
        d->fsWatcher.removeFiles(d->fsWatcher.files());
        const QString gitDir = d->client->findGitDirForRepository(workingDirectory);
        if (!gitDir.isEmpty())
            d->fsWatcher.addFile(gitDir + "/HEAD", Utils::FileSystemWatcher::WatchModifiedDate);
    }
    const QStringList lines = output.split('\n');
    for (const QString &l : lines)
        d->parseOutputLine(l);
    d->flushOldEntries();

    if (d->currentBranch) {
        if (d->currentBranch->isLocal())
            d->currentBranch = nullptr;
        setCurrentBranch();
    }
    if (!d->currentBranch) {
        BranchNode *local = d->rootNode->children.at(LocalBranches);
        d->currentBranch = d->headNode = new BranchNode(tr("Detached HEAD"), "HEAD", QString(),
                                                        d->currentDateTime);
        local->prepend(d->headNode);
    }

    endResetModel();

    return true;
}

void BranchModel::setCurrentBranch()
{
    QString currentBranch = d->client->synchronousCurrentLocalBranch(d->workingDirectory);
    if (currentBranch.isEmpty())
        return;

    BranchNode *local = d->rootNode->children.at(LocalBranches);
    const QStringList branchParts = currentBranch.split('/');
    for (const QString &branchPart : branchParts) {
        local = local->childOfName(branchPart);
        if (!local)
            return;
    }
    d->currentBranch = local;
}

void BranchModel::renameBranch(const QString &oldName, const QString &newName)
{
    QString errorMessage;
    QString output;
    if (!d->client->synchronousBranchCmd(d->workingDirectory, {"-m", oldName,  newName},
                                        &output, &errorMessage))
        VcsOutputWindow::appendError(errorMessage);
    else
        refresh(d->workingDirectory, &errorMessage);
}

void BranchModel::renameTag(const QString &oldName, const QString &newName)
{
    QString errorMessage;
    QString output;
    if (!d->client->synchronousTagCmd(d->workingDirectory, {newName, oldName},
                                     &output, &errorMessage)
            || !d->client->synchronousTagCmd(d->workingDirectory, {"-d", oldName},
                                            &output, &errorMessage)) {
        VcsOutputWindow::appendError(errorMessage);
    } else {
        refresh(d->workingDirectory, &errorMessage);
    }
}

QString BranchModel::workingDirectory() const
{
    return d->workingDirectory;
}

QModelIndex BranchModel::currentBranch() const
{
    if (!d->currentBranch)
        return QModelIndex();
    return nodeToIndex(d->currentBranch, ColumnBranch);
}

QString BranchModel::fullName(const QModelIndex &idx, bool includePrefix) const
{
    if (!idx.isValid())
        return QString();
    BranchNode *node = indexToNode(idx);
    if (!node || !node->isLeaf())
        return QString();
    if (node == d->headNode)
        return QString("HEAD");
    return node->fullRef(includePrefix);
}

QStringList BranchModel::localBranchNames() const
{
    if (!d->rootNode || !d->rootNode->count())
        return QStringList();

    return d->rootNode->children.at(LocalBranches)->childrenNames() + d->obsoleteLocalBranches;
}

QString BranchModel::sha(const QModelIndex &idx) const
{
    if (!idx.isValid())
        return QString();
    BranchNode *node = indexToNode(idx);
    return node->sha;
}

QDateTime BranchModel::dateTime(const QModelIndex &idx) const
{
    if (!idx.isValid())
        return QDateTime();
    BranchNode *node = indexToNode(idx);
    return node->dateTime;
}



bool BranchModel::isHead(const QModelIndex &idx) const
{
    if (!idx.isValid())
        return false;
    BranchNode *node = indexToNode(idx);
    return node == d->headNode;
}

bool BranchModel::isLocal(const QModelIndex &idx) const
{
    if (!idx.isValid())
        return false;
    BranchNode *node = indexToNode(idx);
    return node == d->headNode ? false : node->isLocal();
}

bool BranchModel::isLeaf(const QModelIndex &idx) const
{
    if (!idx.isValid())
        return false;
    BranchNode *node = indexToNode(idx);
    return node->isLeaf();
}

bool BranchModel::isTag(const QModelIndex &idx) const
{
    if (!idx.isValid() || !d->hasTags())
        return false;
    return indexToNode(idx)->isTag();
}

void BranchModel::removeBranch(const QModelIndex &idx)
{
    QString branch = fullName(idx);
    if (branch.isEmpty())
        return;

    QString errorMessage;
    QString output;

    if (!d->client->synchronousBranchCmd(d->workingDirectory, {"-D", branch}, &output, &errorMessage)) {
        VcsOutputWindow::appendError(errorMessage);
        return;
    }
    removeNode(idx);
}

void BranchModel::removeTag(const QModelIndex &idx)
{
    QString tag = fullName(idx);
    if (tag.isEmpty())
        return;

    QString errorMessage;
    QString output;

    if (!d->client->synchronousTagCmd(d->workingDirectory, {"-d", tag}, &output, &errorMessage)) {
        VcsOutputWindow::appendError(errorMessage);
        return;
    }
    removeNode(idx);
}

VcsCommand *BranchModel::checkoutBranch(const QModelIndex &idx)
{
    QString branch = fullName(idx, !isLocal(idx));
    if (branch.isEmpty())
        return nullptr;

    // No StashGuard since this function for now is only used with clean working dir.
    // If it is ever used from another place, please add StashGuard here
    return d->client->checkout(d->workingDirectory, branch, GitClient::StashMode::NoStash);
}

bool BranchModel::branchIsMerged(const QModelIndex &idx)
{
    QString branch = fullName(idx);
    if (branch.isEmpty())
        return false;

    QString errorMessage;
    QString output;

    if (!d->client->synchronousBranchCmd(d->workingDirectory, {"-a", "--contains", sha(idx)},
                                        &output, &errorMessage)) {
        VcsOutputWindow::appendError(errorMessage);
    }

    const QStringList lines = output.split('\n', Qt::SkipEmptyParts);
    for (const QString &l : lines) {
        QString currentBranch = l.mid(2); // remove first letters (those are either
                                          // "  " or "* " depending on whether it is
                                          // the currently checked out branch or not)
        if (currentBranch != branch)
            return true;
    }
    return false;
}

static int positionForName(BranchNode *node, const QString &name)
{
    int pos = 0;
    for (pos = 0; pos < node->count(); ++pos) {
        if (node->children.at(pos)->name >= name)
            break;
    }
    return pos;
}

QModelIndex BranchModel::addBranch(const QString &name, bool track, const QModelIndex &startPoint)
{
    if (!d->rootNode || !d->rootNode->count())
        return QModelIndex();

    const QString trackedBranch = fullName(startPoint);
    const QString fullTrackedBranch = fullName(startPoint, true);
    QString startSha;
    QString output;
    QString errorMessage;
    QDateTime branchDateTime;

    QStringList args = {QLatin1String(track ? "--track" : "--no-track"), name};
    if (!fullTrackedBranch.isEmpty()) {
        args << fullTrackedBranch;
        startSha = sha(startPoint);
        branchDateTime = dateTime(startPoint);
    } else {
        const QStringList arguments({"-n1", "--format=%H %ct"});
        if (d->client->synchronousLog(d->workingDirectory, arguments, &output, &errorMessage,
                                      VcsCommand::SuppressCommandLogging)) {
            const QStringList values = output.split(' ');
            startSha = values[0];
            branchDateTime = QDateTime::fromSecsSinceEpoch(values[1].toLongLong());
        }
    }

    if (!d->client->synchronousBranchCmd(d->workingDirectory, args, &output, &errorMessage)) {
        VcsOutputWindow::appendError(errorMessage);
        return QModelIndex();
    }

    BranchNode *local = d->rootNode->children.at(LocalBranches);
    const int slash = name.indexOf('/');
    const QString leafName = slash == -1 ? name : name.mid(slash + 1);
    bool added = false;
    if (slash != -1) {
        const QString nodeName = name.left(slash);
        int pos = positionForName(local, nodeName);
        BranchNode *child = (pos == local->count()) ? nullptr : local->children.at(pos);
        if (!child || child->name != nodeName) {
            child = new BranchNode(nodeName);
            beginInsertRows(nodeToIndex(local, ColumnBranch), pos, pos);
            added = true;
            child->parent = local;
            local->children.insert(pos, child);
        }
        local = child;
    }
    int pos = positionForName(local, leafName);
    auto newNode = new BranchNode(leafName, startSha, track ? trackedBranch : QString(),
                                  branchDateTime);
    if (!added)
        beginInsertRows(nodeToIndex(local, ColumnBranch), pos, pos);
    newNode->parent = local;
    local->children.insert(pos, newNode);
    endInsertRows();
    return nodeToIndex(newNode, ColumnBranch);
}

void BranchModel::setRemoteTracking(const QModelIndex &trackingIndex)
{
    QModelIndex current = currentBranch();
    QTC_ASSERT(current.isValid(), return);
    const QString currentName = fullName(current);
    const QString shortTracking = fullName(trackingIndex);
    const QString tracking = fullName(trackingIndex, true);
    d->client->synchronousSetTrackingBranch(d->workingDirectory, currentName, tracking);
    d->currentBranch->tracking = shortTracking;
    updateUpstreamStatus(d->currentBranch);
    emit dataChanged(current, current);
}

void BranchModel::setOldBranchesIncluded(bool value)
{
    d->oldBranchesIncluded = value;
}

Utils::optional<QString> BranchModel::remoteName(const QModelIndex &idx) const
{
    const BranchNode *remotesNode = d->rootNode->children.at(RemoteBranches);
    const BranchNode *node = indexToNode(idx);
    if (!node)
        return Utils::nullopt;
    if (node == remotesNode)
        return QString();
    if (node->parent == remotesNode)
        return node->name;
    return Utils::nullopt;
}

void BranchModel::refreshCurrentBranch()
{
    const QModelIndex currentIndex = currentBranch();
    BranchNode *node = indexToNode(currentIndex);
    updateUpstreamStatus(node);
}

void BranchModel::Private::parseOutputLine(const QString &line, bool force)
{
    if (line.size() < 3)
        return;

    // objectname, refname, upstream:short, *objectname, committerdate:raw, *committerdate:raw
    QStringList lineParts = line.split('\t');
    const QString shaDeref = lineParts.at(3);
    const QString sha = shaDeref.isEmpty() ? lineParts.at(0) : shaDeref;
    const QString fullName = lineParts.at(1);
    const QString upstream = lineParts.at(2);
    QDateTime dateTime;
    const bool current = (sha == currentSha);
    QString strDateTime = lineParts.at(5);
    if (strDateTime.isEmpty())
        strDateTime = lineParts.at(4);
    if (!strDateTime.isEmpty()) {
        const qint64 timeT = strDateTime.leftRef(strDateTime.indexOf(' ')).toLongLong();
        dateTime = QDateTime::fromSecsSinceEpoch(timeT);
    }

    bool isOld = false;
    if (!oldBranchesIncluded && !force && !current && dateTime.isValid()) {
        const qint64 age = dateTime.daysTo(QDateTime::currentDateTime());
        isOld = age > Constants::OBSOLETE_COMMIT_AGE_IN_DAYS;
    }
    bool showTags = client->settings().boolValue(GitSettings::showTagsKey);

    // insert node into tree:
    QStringList nameParts = fullName.split('/');
    nameParts.removeFirst(); // remove refs...

    BranchNode *root = nullptr;
    BranchNode *oldEntriesRoot = nullptr;
    RootNodes rootType;
    if (nameParts.first() == "heads") {
        rootType = LocalBranches;
        if (isOld)
            obsoleteLocalBranches.append(fullName.mid(sizeof("refs/heads/")-1));
    } else if (nameParts.first() == "remotes") {
        rootType = RemoteBranches;
        const QString remoteName = nameParts.at(1);
        root = rootNode->children.at(rootType);
        oldEntriesRoot = root->childOfName(remoteName);
        if (!oldEntriesRoot)
            oldEntriesRoot = root->append(new BranchNode(remoteName));
    } else if (showTags && nameParts.first() == "tags") {
        if (!hasTags()) // Tags is missing, add it
            rootNode->append(new BranchNode(tr("Tags"), "refs/tags"));
        rootType = Tags;
    } else {
        return;
    }

    root = rootNode->children.at(rootType);
    if (!oldEntriesRoot)
        oldEntriesRoot = root;
    if (isOld) {
        if (oldEntriesRoot->children.size() > Constants::MAX_OBSOLETE_COMMITS_TO_DISPLAY)
            return;
        if (currentRoot != oldEntriesRoot) {
            flushOldEntries();
            currentRoot = oldEntriesRoot;
        }
        const bool eraseOldestEntry = oldEntries.size() >= Constants::MAX_OBSOLETE_COMMITS_TO_DISPLAY;
        if (!eraseOldestEntry || dateTime > oldEntries.begin()->dateTime) {
            if (eraseOldestEntry)
                oldEntries.erase(oldEntries.begin());
            oldEntries.insert(Private::OldEntry{line, dateTime});
        }
        return;
    }
    nameParts.removeFirst();

    // limit depth of list. Git basically only ever wants one / and considers the rest as part of
    // the name.
    while (nameParts.count() > 3) {
        nameParts[2] = nameParts.at(2) + '/' + nameParts.at(3);
        nameParts.removeAt(3);
    }

    const QString name = nameParts.last();
    nameParts.removeLast();

    auto newNode = new BranchNode(name, sha, upstream, dateTime);
    root->insert(nameParts, newNode);
    if (current)
        currentBranch = newNode;
    q->updateUpstreamStatus(newNode);
}

void BranchModel::Private::flushOldEntries()
{
    if (!currentRoot)
        return;
    for (int size = currentRoot->children.size(); size > 0 && !oldEntries.empty(); --size)
        oldEntries.erase(oldEntries.begin());
    for (const Private::OldEntry &entry : oldEntries)
        parseOutputLine(entry.line, true);
    oldEntries.clear();
    currentRoot = nullptr;
}

BranchNode *BranchModel::indexToNode(const QModelIndex &index) const
{
    if (index.column() > 1)
        return nullptr;
    if (!index.isValid())
        return d->rootNode;
    return static_cast<BranchNode *>(index.internalPointer());
}

QModelIndex BranchModel::nodeToIndex(BranchNode *node, int column) const
{
    if (node == d->rootNode)
        return QModelIndex();
    return createIndex(node->parent->rowOf(node), column, static_cast<void *>(node));
}

void BranchModel::removeNode(const QModelIndex &idx)
{
    QModelIndex nodeIndex = idx; // idx is a leaf, so count must be 0.
    BranchNode *node = indexToNode(nodeIndex);
    while (node->count() == 0 && node->parent != d->rootNode) {
        BranchNode *parentNode = node->parent;
        const QModelIndex parentIndex = nodeToIndex(parentNode, ColumnBranch);
        const int nodeRow = nodeIndex.row();
        beginRemoveRows(parentIndex, nodeRow, nodeRow);
        parentNode->children.removeAt(nodeRow);
        delete node;
        endRemoveRows();
        node = parentNode;
        nodeIndex = parentIndex;
    }
}

void BranchModel::updateUpstreamStatus(BranchNode *node)
{
    if (node->tracking.isEmpty())
        return;
    VcsCommand *command = d->client->asyncUpstreamStatus(
                d->workingDirectory, node->fullRef(), node->tracking);
    QObject::connect(command, &VcsCommand::stdOutText, node, [this, node](const QString &text) {
        const QStringList split = text.trimmed().split('\t');
        QTC_ASSERT(split.size() == 2, return);

        node->setUpstreamStatus(UpstreamStatus(split.at(0).toInt(), split.at(1).toInt()));
        const QModelIndex idx = nodeToIndex(node, ColumnBranch);
        emit dataChanged(idx, idx);
    });
}

QString BranchModel::toolTip(const QString &sha) const
{
    // Show the sha description excluding diff as toolTip
    QString output;
    QString errorMessage;
    QStringList arguments("-n1");
    arguments << sha;
    if (!d->client->synchronousLog(d->workingDirectory, arguments, &output, &errorMessage,
                                  VcsCommand::SuppressCommandLogging)) {
        return errorMessage;
    }
    return output;
}

} // namespace Internal
} // namespace Git

