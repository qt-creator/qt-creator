// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "branchmodel.h"

#include "gitclient.h"
#include "gitconstants.h"
#include "gittr.h"

#include <solutions/tasking/tasktreerunner.h>

#include <utils/environment.h>
#include <utils/qtcprocess.h>
#include <utils/qtcassert.h>
#include <utils/stringutils.h>

#include <vcsbase/vcscommand.h>
#include <vcsbase/vcsoutputwindow.h>

#include <QDateTime>
#include <QFont>
#include <QLoggingCategory>

#include <set>

using namespace Tasking;
using namespace Utils;
using namespace VcsBase;

namespace Git::Internal {

static Q_LOGGING_CATEGORY(nodeLog, "qtc.vcs.git.branches.node", QtWarningMsg);
static Q_LOGGING_CATEGORY(modelLog, "qtc.vcs.git.branches.model", QtWarningMsg);

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
    {
        qCInfo(nodeLog) << "BranchNode created (ROOT)";
    }

    BranchNode(const QString &n, const QString &h = QString(), const QString &t = QString(),
               const QDateTime &dt = QDateTime()) :
        name(n), hash(h), tracking(t), dateTime(dt)
    {
        qCInfo(nodeLog) << "BranchNode created:" << name << hash << tracking << dateTime;
    }

    ~BranchNode()
    {
        qCInfo(nodeLog) << "Destroying BranchNode:" << name << "with" << children.size() << "children";
        while (!children.isEmpty()) {
            qCInfo(nodeLog) << "Deleting child node:" << children.first()->name;
            delete children.first();
        }
        if (parent) {
            parent->children.removeAll(this);
            qCInfo(nodeLog) << "Removed node from parent:" << parent->name;
        }
    }

    BranchNode *rootNode() const
    {
        qCDebug(nodeLog) << "rootNode() called for:" << name;
        return parent ? parent->rootNode() : const_cast<BranchNode *>(this);
    }

    int count() const
    {
        const int result = children.count();
        qCDebug(nodeLog) << "count() called for:" << name << "count:" << result;
        return result;
    }

    bool isLeaf() const
    {
        const bool leaf = children.isEmpty() && parent && parent->parent;
        qCDebug(nodeLog) << "isLeaf() called for:" << name << "Result:" << leaf;
        return leaf;
    }

    bool childOf(BranchNode *node) const
    {
        QTC_ASSERT(node, return false);
        qCDebug(nodeLog) << "childOf() called: this=" << name << "node=" << node->name;

        if (this == node)
            return true;
        return parent ? parent->childOf(node) : false;
    }

    bool childOfRoot(RootNodes root) const
    {
        BranchNode *rn = rootNode();
        QTC_ASSERT(rn, return false);

        if (rn->isLeaf()) {
            qCWarning(nodeLog) << "childOfRoot: root node is leaf:" << rn->name;
            return false;
        }
        if (root >= rn->children.count()) {
            qCWarning(nodeLog) << "childOfRoot: root index out of range:" << root << rn->children.count();
            return false;
        }
        const bool result = childOf(rn->children.at(root));
        qCDebug(nodeLog) << "childOfRoot() called for:" << name << "Result:" << result;
        return result;
    }

    bool isTag() const
    {
        const bool result = childOfRoot(Tags);
        qCDebug(nodeLog) << "isTag() called for:" << name << "Result:" << result;
        return result;
    }

    bool isLocal() const
    {
        const bool result = childOfRoot(LocalBranches);
        qCDebug(nodeLog) << "isLocal() called for:" << name << "Result:" << result;
        return result;
    }

    BranchNode *childOfName(const QString &name) const
    {
        qCDebug(nodeLog) << "childOfName() called for:" << this->name << "searching for:" << name;
        for (int i = 0; i < children.count(); ++i) {
            if (children.at(i)->name == name) {
                qCDebug(nodeLog) << "childOfName: found child:" << name;
                return children.at(i);
            }
        }
        qCDebug(nodeLog) << "childOfName: not found:" << name;
        return nullptr;
    }

    QStringList fullName(bool includePrefix = false) const
    {
        qCDebug(nodeLog) << "fullName() called for:" << name << "includePrefix:" << includePrefix;
        QStringList fn;
        QList<const BranchNode *> nodes;
        const BranchNode *current = this;
        while (current->parent) {
            nodes.prepend(current);
            current = current->parent;
        }

        if (includePrefix)
            fn.append(nodes.first()->hash);
        nodes.removeFirst();

        for (const BranchNode *n : std::as_const(nodes))
            fn.append(n->name);

        qCDebug(nodeLog) << "fullName for" << name << ":" << fn;
        return fn;
    }

    QString fullRef(bool includePrefix = false) const
    {
        const QString ref = fullName(includePrefix).join('/');
        qCDebug(nodeLog) << "fullRef() called for:" << name << "Result:" << ref;
        return ref;
    }

    void insert(const QStringList &path, BranchNode *n)
    {
        QTC_ASSERT(n, return);
        qCDebug(nodeLog) << "insert() called for:" << name << "path:" << path << "node:" << n->name;

        BranchNode *current = this;
        for (int i = 0; i < path.count(); ++i) {
            BranchNode *c = current->childOfName(path.at(i));
            if (c) {
                qCDebug(nodeLog) << "insert: found existing child:" << c->name;
                current = c;
            } else {
                qCDebug(nodeLog) << "insert: creating new child:" << path.at(i);
                current = current->append(new BranchNode(path.at(i)));
            }
        }
        qCDebug(nodeLog) << "insert: appending node:" << n->name << "to:" << current->name;
        current->append(n);
    }

    BranchNode *append(BranchNode *n)
    {
        QTC_ASSERT(n, return nullptr);
        qCDebug(nodeLog) << "append() called for:" << name << "appending:" << n->name;

        n->parent = this;
        children.append(n);
        return n;
    }

    BranchNode *prepend(BranchNode *n)
    {
        QTC_ASSERT(n, return nullptr);
        qCDebug(nodeLog) << "prepend() called for:" << name << "prepending:" << n->name;

        n->parent = this;
        children.prepend(n);
        return n;
    }

    QStringList childrenNames() const
    {
        qCDebug(nodeLog) << "childrenNames() called for:" << name;
        if (!children.isEmpty()) {
            QStringList names;
            for (BranchNode *n : children) {
                names.append(n->childrenNames());
            }
            return names;
        }
        const QStringList result = {fullRef()};
        qCDebug(nodeLog) << "childrenNames: leaf node, returning:" << result;
        return result;
    }

    int rowOf(BranchNode *node)
    {
        QTC_ASSERT(node, return -1);
        const int idx = children.indexOf(node);
        qCDebug(nodeLog) << "rowOf() called for:" << name << "searching for:"
                         << node->name << "Result:" << idx;
        return idx;
    }

    void setUpstreamStatus(UpstreamStatus newStatus)
    {
        qCDebug(nodeLog) << "setUpstreamStatus() called for:" << name
                         << "Old status: (" << status.ahead << "," << status.behind << "),"
                         << "New status: (" << newStatus.ahead << "," << newStatus.behind << ")";
        status = newStatus;
    }

    BranchNode *parent = nullptr;
    QList<BranchNode *> children;

    QString name;
    QString hash;
    QString tracking;
    QDateTime dateTime;
    UpstreamStatus status;
    mutable QString toolTip;
};

class BranchModel::Private
{
public:
    explicit Private(BranchModel *q) :
        q(q),
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
    void updateAllUpstreamStatus(BranchNode *node);

    BranchModel *q = nullptr;
    FilePath workingDirectory;
    BranchNode *rootNode = nullptr;
    BranchNode *currentBranch = nullptr;
    BranchNode *headNode = nullptr;
    QString currentHash;
    QDateTime currentDateTime;
    QStringList obsoleteLocalBranches;
    TaskTreeRunner taskTreeRunner;
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

BranchModel::BranchModel(QObject *parent) :
    QAbstractItemModel(parent),
    d(new Private(this))
{
    qCInfo(modelLog) << "BranchModel constructed";
    // Abuse the hash field for ref prefix
    d->rootNode->append(new BranchNode(Tr::tr("Local Branches"), "refs/heads"));
    d->rootNode->append(new BranchNode(Tr::tr("Remote Branches"), "refs/remotes"));
    connect(&d->taskTreeRunner, &TaskTreeRunner::done, this, &BranchModel::endResetModel);
}

BranchModel::~BranchModel()
{
    qCInfo(modelLog) << "BranchModel destructed";
    delete d;
}

QModelIndex BranchModel::index(int row, int column, const QModelIndex &parentIdx) const
{
    qCDebug(modelLog) << "index() called: row=" << row << "column=" << column << "parentIdx=" << parentIdx;
    if (column > 1)
        return {};
    BranchNode *parentNode = indexToNode(parentIdx);
    QTC_ASSERT(parentNode, return {});

    if (row >= parentNode->count()) {
        qCWarning(modelLog) << "index: row out of range:" << row << "parent node:" << parentNode->name;
        return {};
    }
    BranchNode *childNode = parentNode->children.at(row);
    QTC_ASSERT(childNode, return {});
    const QModelIndex result = nodeToIndex(childNode, column);
    qCDebug(modelLog) << "index: returning index for node:" << childNode->name
                      << "column:" << column;
    return result;
}

QModelIndex BranchModel::parent(const QModelIndex &index) const
{
    qCDebug(modelLog) << "parent() called for index:" << index;
    if (!index.isValid())
        return {};

    BranchNode *node = indexToNode(index);
    QTC_ASSERT(node, return {});
    BranchNode *parentNode = node->parent;
    QTC_ASSERT(parentNode, return {});
    if (parentNode == d->rootNode) {
        qCDebug(modelLog) << "parent: node is direct child of root, returning empty";
        return {};
    }
    const QModelIndex parentIndex = nodeToIndex(parentNode, ColumnBranch);
    qCDebug(modelLog) << "parent: returning parent index for node:" << parentNode->name;
    return parentIndex;
}

int BranchModel::rowCount(const QModelIndex &parentIdx) const
{
    qCDebug(modelLog) << "rowCount() called for parentIdx:" << parentIdx;
    if (parentIdx.column() > 0)
        return 0;

    const BranchNode *node = indexToNode(parentIdx);
    QTC_ASSERT(node, return 0);

    const int result = node->count();
    qCDebug(modelLog) << "rowCount: node:" << node->name << "count:" << result;
    return result;
}

int BranchModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    qCDebug(modelLog) << "columnCount() called, returning:" << ColumnCount;
    return ColumnCount;
}

QVariant BranchModel::data(const QModelIndex &index, int role) const
{
    qCDebug(modelLog) << "data() called: index=" << index << "role=" << role;
    const QChar arrowUp(0x2191);
    const QChar arrowDown(0x2193);

    BranchNode *node = indexToNode(index);
    if (!node) {
        qCWarning(modelLog) << "data: invalid node for index:" << index;
        return {};
    }

    switch (role) {
    case Qt::DisplayRole: {
        QString res;
        switch (index.column()) {
        case ColumnBranch: {
            res = node->name;
            if (!node->isLocal() || !node->isLeaf())
                break;

            if (node->status.ahead >= 0)
                res += ' ' + arrowUp + QString::number(node->status.ahead);

            if (!node->tracking.isEmpty()) {
                if (node->status.behind >= 0)
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
        qCDebug(modelLog) << "data: DisplayRole for node:" << node->name << "column:" << index.column() << "result:" << res;
        return res;
    }
    case Qt::EditRole:
        qCDebug(modelLog) << "data: EditRole for node:" << node->name;
        return index.column() == 0 ? node->fullRef() : QVariant();
    case Qt::ToolTipRole:
        if (!node->isLeaf())
            return {};
        if (node->toolTip.isEmpty())
            node->toolTip = toolTip(node->hash);
        qCDebug(modelLog) << "data: ToolTipRole for node:" << node->name << "toolTip:" << node->toolTip;
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
        qCDebug(modelLog) << "data: FontRole for node:" << node->name << "font:" << font;
        return font;
    }
    default:
        return {};
    }
}

bool BranchModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    qCDebug(modelLog) << "setData() called: index=" << index << "value=" << value << "role=" << role;
    if (index.column() != ColumnBranch || role != Qt::EditRole)
        return false;
    BranchNode *node = indexToNode(index);
    if (!node) {
        qCWarning(modelLog) << "setData: invalid node for index:" << index;
        return false;
    }

    const QString newName = value.toString();
    if (newName.isEmpty())
        return false;

    const QString oldName = node->fullRef();
    if (oldName == newName)
        return false;

    qCDebug(modelLog) << "setData: renaming branch from" << oldName << "to" << newName;
    renameBranch(oldName, newName);
    return true;
}

Qt::ItemFlags BranchModel::flags(const QModelIndex &index) const
{
    qCDebug(modelLog) << "flags() called: index=" << index;
    BranchNode *node = indexToNode(index);
    if (!node) {
        qCWarning(modelLog) << "flags: invalid node for index:" << index;
        return Qt::NoItemFlags;
    }
    Qt::ItemFlags res = Qt::ItemIsSelectable | Qt::ItemIsEnabled;
    if (node != d->headNode && node->isLeaf() && node->isLocal() && index.column() == ColumnBranch)
        res |= Qt::ItemIsEditable;
    qCDebug(modelLog) << "flags: node=" << node->name << "flags=" << res;
    return res;
}

void BranchModel::clear()
{
    qCDebug(modelLog) << "clear() called";
    for (BranchNode *root : std::as_const(d->rootNode->children)) {
        QTC_ASSERT(root, continue);

        while (root->count()) {
            qCDebug(modelLog) << "clear: deleting child node:" << root->children.last()->name;
            delete root->children.takeLast();
        }
    }
    if (d->hasTags()) {
        qCDebug(modelLog) << "clear: removing tags node";
        d->rootNode->children.takeLast();
    }

    d->currentHash.clear();
    d->currentDateTime = {};
    d->currentBranch = nullptr;
    d->headNode = nullptr;
    d->obsoleteLocalBranches.clear();
    qCDebug(modelLog) << "clear: model state reset";
}

void BranchModel::refresh(const FilePath &workingDirectory, ShowError showError)
{
    if (d->taskTreeRunner.isRunning()) {
        endResetModel(); // for the running task tree.
        d->taskTreeRunner.reset(); // old running tree is reset, no handlers are being called
    }
    beginResetModel();
    clear();
    if (workingDirectory.isEmpty()) {
        endResetModel();
        return;
    }

    const GroupItem topRevisionProc = gitClient().topRevision(workingDirectory,
        [this](const QString &ref, const QDateTime &dateTime) {
            d->currentHash = ref;
            d->currentDateTime = dateTime;
        });

    const auto onForEachRefSetup = [this, workingDirectory](Process &process) {
        d->workingDirectory = workingDirectory;
        QStringList args = {"for-each-ref",
                            "--format=%(objectname)\t%(refname)\t%(upstream:short)\t"
                            "%(*objectname)\t%(committerdate:raw)\t%(*committerdate:raw)",
                            "refs/heads/**",
                            "refs/remotes/**"};
        if (settings().showTags())
            args << "refs/tags/**";
        gitClient().setupCommand(process, workingDirectory, args);
    };

    const auto onForEachRefDone = [this, workingDirectory, showError](const Process &process,
                                                                      DoneWith result) {
        if (result != DoneWith::Success) {
            if (showError == ShowError::No)
                return;
            const QString message = Tr::tr("Cannot run \"%1\" in \"%2\": %3")
                                        .arg("git for-each-ref")
                                        .arg(workingDirectory.toUserOutput())
                                        .arg(process.cleanedStdErr());
            VcsBase::VcsOutputWindow::appendError(message);
            return;
        }
        const QString output = process.stdOut();
        const QStringList lines = output.split('\n');
        for (const QString &l : lines)
            d->parseOutputLine(l);
        d->flushOldEntries();

        d->updateAllUpstreamStatus(d->rootNode->children.at(LocalBranches));
        if (d->currentBranch) {
            if (d->currentBranch->isLocal())
                d->currentBranch = nullptr;
            setCurrentBranch();
        }
        if (!d->currentBranch) {
            BranchNode *local = d->rootNode->children.at(LocalBranches);
            QTC_ASSERT(local, return);
            d->currentBranch = d->headNode = new BranchNode(
                Tr::tr("Detached HEAD"), "HEAD", {}, d->currentDateTime);
            local->prepend(d->headNode);
        }
    };

    const Group root {
        topRevisionProc,
        ProcessTask(onForEachRefSetup, onForEachRefDone)
    };
    d->taskTreeRunner.start(root);
}

void BranchModel::setCurrentBranch()
{
    const QString currentBranch = gitClient().synchronousCurrentLocalBranch(d->workingDirectory);
    if (currentBranch.isEmpty())
        return;

    BranchNode *local = d->rootNode->children.at(LocalBranches);
    QTC_ASSERT(local, return);

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
    if (!gitClient().synchronousBranchCmd(d->workingDirectory, {"-m", oldName,  newName},
                                          &output, &errorMessage))
        VcsOutputWindow::appendError(errorMessage);
    else
        refresh(d->workingDirectory);
}

void BranchModel::renameTag(const QString &oldName, const QString &newName)
{
    QString errorMessage;
    QString output;
    if (!gitClient().synchronousTagCmd(d->workingDirectory, {newName, oldName},
                                       &output, &errorMessage)
            || !gitClient().synchronousTagCmd(d->workingDirectory, {"-d", oldName},
                                          &output, &errorMessage)) {
        VcsOutputWindow::appendError(errorMessage);
    } else {
        refresh(d->workingDirectory);
    }
}

FilePath BranchModel::workingDirectory() const
{
    return d->workingDirectory;
}

QModelIndex BranchModel::currentBranch() const
{
    if (!d->currentBranch)
        return {};
    return nodeToIndex(d->currentBranch, ColumnBranch);
}

QString BranchModel::fullName(const QModelIndex &idx, bool includePrefix) const
{
    if (!idx.isValid())
        return {};
    BranchNode *node = indexToNode(idx);
    if (!node || !node->isLeaf())
        return {};
    if (node == d->headNode)
        return QString("HEAD");
    return node->fullRef(includePrefix);
}

QStringList BranchModel::localBranchNames() const
{
    qCDebug(modelLog) << "localBranchNames() called";
    if (!d->rootNode || !d->rootNode->count()) {
        qCWarning(modelLog) << "localBranchNames: no root node or no children";
        return {};
    }
    QStringList names = d->rootNode->children.at(LocalBranches)->childrenNames() + d->obsoleteLocalBranches;
    qCDebug(modelLog) << "localBranchNames: returning" << names;
    return names;
}

QString BranchModel::hash(const QModelIndex &idx) const
{
    qCDebug(modelLog) << "hash() called: idx=" << idx;
    if (!idx.isValid())
        return {};
    BranchNode *node = indexToNode(idx);
    QTC_ASSERT(node, return {});
    qCDebug(modelLog) << "hash: node=" << node->name << "hash=" << node->hash;
    return node->hash;
}

QDateTime BranchModel::dateTime(const QModelIndex &idx) const
{
    qCDebug(modelLog) << "dateTime() called: idx=" << idx;
    if (!idx.isValid())
        return {};
    BranchNode *node = indexToNode(idx);
    QTC_ASSERT(node, return {});
    qCDebug(modelLog) << "dateTime: node=" << node->name << "dateTime=" << node->dateTime;
    return node->dateTime;
}

bool BranchModel::isHead(const QModelIndex &idx) const
{
    qCDebug(modelLog) << "isHead() called: idx=" << idx;
    if (!idx.isValid())
        return false;
    BranchNode *node = indexToNode(idx);
    QTC_ASSERT(node, return false);
    bool result = node == d->headNode;
    qCDebug(modelLog) << "isHead: node=" << node->name << "isHead=" << result;
    return result;
}

bool BranchModel::isLocal(const QModelIndex &idx) const
{
    qCDebug(modelLog) << "isLocal() called: idx=" << idx;
    if (!idx.isValid())
        return false;
    BranchNode *node = indexToNode(idx);
    QTC_ASSERT(node, return false);
    bool result = node == d->headNode ? false : node->isLocal();
    qCDebug(modelLog) << "isLocal: node=" << node->name << "isLocal=" << result;
    return result;
}

bool BranchModel::isLeaf(const QModelIndex &idx) const
{
    qCDebug(modelLog) << "isLeaf() called: idx=" << idx;
    if (!idx.isValid())
        return false;
    BranchNode *node = indexToNode(idx);
    QTC_ASSERT(node, return false);
    bool result = node->isLeaf();
    qCDebug(modelLog) << "isLeaf: node=" << node->name << "isLeaf=" << result;
    return result;
}

bool BranchModel::isTag(const QModelIndex &idx) const
{
    qCDebug(modelLog) << "isTag() called: idx=" << idx;
    if (!idx.isValid() || !d->hasTags())
        return false;
    const BranchNode *node = indexToNode(idx);
    QTC_ASSERT(node, return false);
    const bool result = node->isTag();
    qCDebug(modelLog) << "isTag: node=" << node->name << "isTag=" << result;
    return result;
}

void BranchModel::removeBranch(const QModelIndex &idx)
{
    qCDebug(modelLog) << "removeBranch() called: idx=" << idx;
    const QString branch = fullName(idx);
    if (branch.isEmpty()) {
        qCWarning(modelLog) << "removeBranch: branch name is empty for idx=" << idx;
        return;
    }

    QString errorMessage;
    QString output;

    if (!gitClient().synchronousBranchCmd(d->workingDirectory, {"-D", branch}, &output, &errorMessage)) {
        qCWarning(modelLog) << "removeBranch: git branch delete failed:" << errorMessage;
        VcsOutputWindow::appendError(errorMessage);
        return;
    }
    qCDebug(modelLog) << "removeBranch: branch deleted successfully:" << branch;
    removeNode(idx);
}

void BranchModel::removeTag(const QModelIndex &idx)
{
    qCDebug(modelLog) << "removeTag() called: idx=" << idx;
    const QString tag = fullName(idx);
    if (tag.isEmpty()) {
        qCWarning(modelLog) << "removeTag: tag name is empty for idx=" << idx;
        return;
    }

    QString errorMessage;
    QString output;

    if (!gitClient().synchronousTagCmd(d->workingDirectory, {"-d", tag}, &output, &errorMessage)) {
        qCWarning(modelLog) << "removeTag: git tag delete failed:" << errorMessage;
        VcsOutputWindow::appendError(errorMessage);
        return;
    }
    qCDebug(modelLog) << "removeTag: tag deleted successfully:" << tag;
    removeNode(idx);
}

void BranchModel::checkoutBranch(const QModelIndex &idx, const QObject *context,
                                 const CommandHandler &handler)
{
    qCDebug(modelLog) << "checkoutBranch() called: idx=" << idx;
    const QString branch = fullName(idx, !isLocal(idx));
    if (branch.isEmpty()) {
        qCWarning(modelLog) << "checkoutBranch: branch name is empty for idx=" << idx;
        return;
    }

    qCDebug(modelLog) << "checkoutBranch: checking out branch:" << branch;
    // No StashGuard since this function for now is only used with clean working dir.
    // If it is ever used from another place, please add StashGuard here
    gitClient().checkout(d->workingDirectory, branch, GitClient::StashMode::NoStash,
                         context, handler);
}

bool BranchModel::branchIsMerged(const QModelIndex &idx)
{
    qCDebug(modelLog) << "branchIsMerged() called: idx=" << idx;
    const QString branch = fullName(idx);
    if (branch.isEmpty()) {
        qCWarning(modelLog) << "branchIsMerged: branch name is empty for idx=" << idx;
        return false;
    }

    QString errorMessage;
    QString output;

    if (!gitClient().synchronousBranchCmd(d->workingDirectory, {"-a", "--contains", hash(idx)},
                                          &output, &errorMessage)) {
        qCWarning(modelLog) << "branchIsMerged: git branch contains failed:" << errorMessage;
        VcsOutputWindow::appendError(errorMessage);
    }

    const QStringList lines = output.split('\n', Qt::SkipEmptyParts);
    for (const QString &l : lines) {
        const QString currentBranch = l.mid(2); // remove first letters (those are either
            // "  " or "* " depending on whether it is
            // the currently checked out branch or not)
        if (currentBranch != branch) {
            qCDebug(modelLog) << "branchIsMerged: found merged branch:" << currentBranch;
            return true;
        }
    }
    qCDebug(modelLog) << "branchIsMerged: branch is not merged:" << branch;
    return false;
}

static int positionForName(BranchNode *node, const QString &name)
{
    QTC_ASSERT(node, return 0);
    qCDebug(modelLog) << "positionForName() called: node=" << node->name << "name=" << name;
    int pos = 0;
    for (pos = 0; pos < node->count(); ++pos) {
        if (node->children.at(pos)->name >= name)
            break;
    }
    qCDebug(modelLog) << "positionForName: result pos=" << pos;
    return pos;
}

QModelIndex BranchModel::addBranch(const QString &name, bool track, const QModelIndex &startPoint)
{
    qCDebug(modelLog) << "addBranch() called: name=" << name << "track=" << track << "startPoint=" << startPoint;
    if (!d->rootNode || !d->rootNode->count()) {
        qCWarning(modelLog) << "addBranch: no root node or no children";
        return {};
    }

    const QString trackedBranch = fullName(startPoint);
    const QString fullTrackedBranch = fullName(startPoint, true);
    QString startHash;
    QString errorMessage;
    QDateTime branchDateTime;

    QStringList args = {QLatin1String(track ? "--track" : "--no-track"), name};
    if (!fullTrackedBranch.isEmpty()) {
        args << fullTrackedBranch;
        startHash = hash(startPoint);
        branchDateTime = dateTime(startPoint);
        qCDebug(modelLog) << "addBranch: tracking branch" << fullTrackedBranch << "hash=" << startHash << "dateTime=" << branchDateTime;
    } else {
        const Result<QString> res = gitClient().synchronousLog(d->workingDirectory,
                                                               {"-n1", "--format=%H %ct"},
                                                               RunFlags::SuppressCommandLogging);
        if (res) {
            const QStringList values = res.value().split(' ');
            startHash = values[0];
            branchDateTime = QDateTime::fromSecsSinceEpoch(values[1].toLongLong());
            qCDebug(modelLog) << "addBranch: fallback hash=" << startHash << "dateTime=" << branchDateTime;
        } else {
            errorMessage = res.error();
            qCWarning(modelLog) << "addBranch: failed to get fallback hash:" << errorMessage;
        }
    }

    QString output;

    if (!gitClient().synchronousBranchCmd(d->workingDirectory, args, &output, &errorMessage)) {
        qCWarning(modelLog) << "addBranch: git branch creation failed:" << errorMessage;
        VcsOutputWindow::appendError(errorMessage);
        return {};
    }

    BranchNode *local = d->rootNode->children.at(LocalBranches);
    QTC_ASSERT(local, return {});

    const int slash = name.indexOf('/');
    const QString leafName = slash == -1 ? name : name.mid(slash + 1);
    bool added = false;
    if (slash != -1) {
        const QString nodeName = name.left(slash);
        int pos = positionForName(local, nodeName);
        BranchNode *child = (pos == local->count()) ? nullptr : local->children.at(pos);
        if (!child || child->name != nodeName) {
            qCDebug(modelLog) << "addBranch: creating intermediate node:" << nodeName << "at pos:" << pos;
            child = new BranchNode(nodeName);
            beginInsertRows(nodeToIndex(local, ColumnBranch), pos, pos);
            added = true;
            child->parent = local;
            local->children.insert(pos, child);
        }
        local = child;
    }
    int pos = positionForName(local, leafName);
    qCDebug(modelLog) << "addBranch: inserting new branch node:" << leafName << "at pos:" << pos;
    auto newNode = new BranchNode(leafName, startHash, track ? trackedBranch : QString(),
                                  branchDateTime);
    if (!added)
        beginInsertRows(nodeToIndex(local, ColumnBranch), pos, pos);
    newNode->parent = local;
    local->children.insert(pos, newNode);
    endInsertRows();
    qCDebug(modelLog) << "addBranch: branch added successfully:" << leafName;
    return nodeToIndex(newNode, ColumnBranch);
}

void BranchModel::setRemoteTracking(const QModelIndex &trackingIndex)
{
    qCDebug(modelLog) << "setRemoteTracking() called: trackingIndex=" << trackingIndex;
    QModelIndex current = currentBranch();
    QTC_ASSERT(current.isValid(), return);
    const QString currentName = fullName(current);
    const QString shortTracking = fullName(trackingIndex);
    const QString tracking = fullName(trackingIndex, true);
    qCDebug(modelLog) << "setRemoteTracking: currentName=" << currentName << "shortTracking=" << shortTracking << "tracking=" << tracking;
    gitClient().synchronousSetTrackingBranch(d->workingDirectory, currentName, tracking);
    d->currentBranch->tracking = shortTracking;
    updateUpstreamStatus(d->currentBranch);
    emit dataChanged(current, current);
    qCDebug(modelLog) << "setRemoteTracking: tracking branch set and dataChanged emitted";
}

void BranchModel::setOldBranchesIncluded(bool value)
{
    qCDebug(modelLog) << "setOldBranchesIncluded() called: value=" << value;
    d->oldBranchesIncluded = value;
}

std::optional<QString> BranchModel::remoteName(const QModelIndex &idx) const
{
    qCDebug(modelLog) << "remoteName() called: idx=" << idx;
    const BranchNode *remotesNode = d->rootNode->children.at(RemoteBranches);
    QTC_ASSERT(remotesNode, return std::nullopt);

    const BranchNode *node = indexToNode(idx);
    if (!node) {
        qCWarning(modelLog) << "remoteName: node is null for idx=" << idx;
        return std::nullopt;
    }
    if (node == remotesNode) {
        qCDebug(modelLog) << "remoteName: node is remotesNode";
        return QString(); // keep QString() as {} might convert to std::nullopt
    }
    if (node->parent == remotesNode) {
        qCDebug(modelLog) << "remoteName: node is direct child of remotesNode:" << node->name;
        return node->name;
    }
    qCDebug(modelLog) << "remoteName: node is not a remote";
    return std::nullopt;
}

void BranchModel::refreshCurrentBranch()
{
    qCDebug(modelLog) << "refreshCurrentBranch() called";
    const QModelIndex currentIndex = currentBranch();
    BranchNode *node = indexToNode(currentIndex);
    QTC_ASSERT(node, return);

    updateUpstreamStatus(node);
    qCDebug(modelLog) << "refreshCurrentBranch: upstream status updated for" << node->name;
}

void BranchModel::Private::parseOutputLine(const QString &line, bool force)
{
    qCDebug(modelLog) << "Private::parseOutputLine() called: line=" << line << "force=" << force;
    if (line.size() < 3)
        return;

    // objectname, refname, upstream:short, *objectname, committerdate:raw, *committerdate:raw
    const QStringList lineParts = line.split('\t');
    const QString hashDeref = lineParts.at(3);
    const QString hash = hashDeref.isEmpty() ? lineParts.at(0) : hashDeref;
    const QString fullName = lineParts.at(1);
    const QString upstream = lineParts.at(2);
    QDateTime dateTime;
    const bool current = (hash == currentHash);
    QString strDateTime = lineParts.at(5);
    if (strDateTime.isEmpty())
        strDateTime = lineParts.at(4);
    if (!strDateTime.isEmpty()) {
        const qint64 timeT = strDateTime.left(strDateTime.indexOf(' ')).toLongLong();
        dateTime = QDateTime::fromSecsSinceEpoch(timeT);
    }

    bool isOld = false;
    if (!oldBranchesIncluded && !force && !current && dateTime.isValid()) {
        const qint64 age = dateTime.daysTo(QDateTime::currentDateTime());
        isOld = age > Constants::OBSOLETE_COMMIT_AGE_IN_DAYS;
    }
    const bool showTags = settings().showTags();

    // insert node into tree:
    QStringList nameParts = fullName.split('/');
    nameParts.removeFirst(); // remove refs...

    BranchNode *root = nullptr;
    BranchNode *oldEntriesRoot = nullptr;
    RootNodes rootType;
    if (nameParts.first() == "heads") {
        rootType = LocalBranches;
        if (isOld) {
            qCDebug(modelLog) << "parseOutputLine: obsolete local branch:" << fullName;
            obsoleteLocalBranches.append(fullName.mid(sizeof("refs/heads/")-1));
        }
    } else if (nameParts.first() == "remotes") {
        rootType = RemoteBranches;
        const QString remoteName = nameParts.at(1);
        root = rootNode->children.at(rootType);
        QTC_ASSERT(root, return);

        oldEntriesRoot = root->childOfName(remoteName);
        if (!oldEntriesRoot) {
            qCDebug(modelLog) << "parseOutputLine: creating remote node:" << remoteName;
            oldEntriesRoot = root->append(new BranchNode(remoteName));
        }
    } else if (showTags && nameParts.first() == "tags") {
        if (!hasTags()) { // Tags is missing, add it
            qCDebug(modelLog) << "parseOutputLine: adding tags node";
            rootNode->append(new BranchNode(Tr::tr("Tags"), "refs/tags"));
        }
        rootType = Tags;
    } else {
        qCWarning(modelLog) << "parseOutputLine: unknown branch type:" << nameParts.first();
        return;
    }

    root = rootNode->children.at(rootType);
    if (!oldEntriesRoot)
        oldEntriesRoot = root;
    if (isOld) {
        QTC_ASSERT(oldEntriesRoot, return);
        if (oldEntriesRoot->children.size() > Constants::MAX_OBSOLETE_COMMITS_TO_DISPLAY) {
            qCDebug(modelLog) << "parseOutputLine: too many obsolete entries, skipping";
            return;
        }
        if (currentRoot != oldEntriesRoot) {
            flushOldEntries();
            currentRoot = oldEntriesRoot;
        }
        const bool eraseOldestEntry = oldEntries.size() >= Constants::MAX_OBSOLETE_COMMITS_TO_DISPLAY;
        if (!eraseOldestEntry || dateTime > oldEntries.begin()->dateTime) {
            if (eraseOldestEntry) {
                qCDebug(modelLog) << "parseOutputLine: erasing oldest obsolete entry";
                oldEntries.erase(oldEntries.begin());
            }
            oldEntries.insert(Private::OldEntry{line, dateTime});
            qCDebug(modelLog) << "parseOutputLine: inserted obsolete entry";
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

    QTC_ASSERT(root, return);
    auto newNode = new BranchNode(name, hash, upstream, dateTime);
    qCDebug(modelLog) << "parseOutputLine: inserting node" << name << "hash=" << hash << "upstream=" << upstream << "dateTime=" << dateTime;
    root->insert(nameParts, newNode);
    if (current) {
        qCDebug(modelLog) << "parseOutputLine: current branch set to" << name;
        currentBranch = newNode;
    }
}

void BranchModel::Private::flushOldEntries()
{
    qCDebug(modelLog) << "Private::flushOldEntries() called";
    if (!currentRoot)
        return;
    for (int size = currentRoot->children.size(); size > 0 && !oldEntries.empty(); --size)
        oldEntries.erase(oldEntries.begin());
    for (const Private::OldEntry &entry : oldEntries) {
        qCDebug(modelLog) << "Private::flushOldEntries: re-parsing old entry";
        parseOutputLine(entry.line, true);
    }
    oldEntries.clear();
    currentRoot = nullptr;
    qCDebug(modelLog) << "Private::flushOldEntries: done";
}

BranchNode *BranchModel::indexToNode(const QModelIndex &index) const
{
    qCDebug(modelLog) << "indexToNode() called: index=" << index;
    if (index.column() > 1)
        return nullptr;
    if (!index.isValid())
        return d->rootNode;
    return static_cast<BranchNode *>(index.internalPointer());
}

QModelIndex BranchModel::nodeToIndex(BranchNode *node, int column) const
{
    QTC_ASSERT(node, return {});
    QTC_ASSERT(node->parent, return {});
    qCDebug(modelLog) << "nodeToIndex() called: node=" << node->name << "column=" << column;
    if (node == d->rootNode)
        return {};
    const QModelIndex idx = createIndex(node->parent->rowOf(node), column, static_cast<void *>(node));
    qCDebug(modelLog) << "nodeToIndex: returning index" << idx;
    return idx;
}

void BranchModel::removeNode(const QModelIndex &idx)
{
    qCDebug(modelLog) << "removeNode() called: idx=" << idx;
    QModelIndex nodeIndex = idx; // idx is a leaf, so count must be 0.
    BranchNode *node = indexToNode(nodeIndex);
    QTC_ASSERT(node, return);

    while (node->count() == 0 && node->parent != d->rootNode) {
        BranchNode *parentNode = node->parent;
        QTC_ASSERT(node, return);
        const QModelIndex parentIndex = nodeToIndex(parentNode, ColumnBranch);
        const int nodeRow = nodeIndex.row();
        qCDebug(modelLog) << "removeNode: removing node" << node->name << "from parent"
                          << parentNode->name << "at row" << nodeRow;
        beginRemoveRows(parentIndex, nodeRow, nodeRow);
        parentNode->children.removeAt(nodeRow);
        delete node;
        endRemoveRows();
        node = parentNode;
        nodeIndex = parentIndex;
    }
    qCDebug(modelLog) << "removeNode: done";
}

void BranchModel::updateUpstreamStatus(BranchNode *node)
{
    if (!node || !node->isLocal())
        return;
    qCDebug(modelLog) << "updateUpstreamStatus() called: node=" << node->name;

    Process *process = new Process(node);
    process->setEnvironment(gitClient().processEnvironment(d->workingDirectory));
    QStringList parameters = {"rev-list", "--no-color", "--count"};
    if (node->tracking.isEmpty())
        parameters += {node->fullRef(), "--not", "--remotes"};
    else
        parameters += {"--left-right", node->fullRef() + "..." + node->tracking};
    process->setCommand({gitClient().vcsBinary(d->workingDirectory), parameters});
    process->setWorkingDirectory(d->workingDirectory);
    qCDebug(modelLog) << "updateUpstreamStatus: starting process with parameters" << parameters;
    connect(process, &Process::done, this, [this, process, node] {
        qCDebug(modelLog) << "updateUpstreamStatus: process done for node" << node->name
                          << "result=" << int(process->result());
        process->deleteLater();
        if (process->result() != ProcessResult::FinishedWithSuccess)
            return;
        const QString text = process->cleanedStdOut();
        if (text.isEmpty())
            return;
        const QStringList split = text.trimmed().split('\t');
        if (node->tracking.isEmpty()) {
            node->setUpstreamStatus(UpstreamStatus(split.at(0).toInt(), 0));
        } else {
            QTC_ASSERT(split.size() == 2, return);

            node->setUpstreamStatus(UpstreamStatus(split.at(0).toInt(), split.at(1).toInt()));
        }
        const QModelIndex idx = nodeToIndex(node, ColumnBranch);
        emit dataChanged(idx, idx);
        qCDebug(modelLog) << "updateUpstreamStatus: dataChanged emitted for node" << node->name;
    });
    process->start();
    qCDebug(modelLog) << "updateUpstreamStatus: process started";
}

void BranchModel::Private::updateAllUpstreamStatus(BranchNode *node)
{
    if (!node) {
        qCDebug(modelLog) << "Private::updateAllUpstreamStatus() called: node=nullptr";
        return;
    }
    qCDebug(modelLog) << "Private::updateAllUpstreamStatus() called: node=" << node->name;

    if (node->isLeaf()) {
        qCDebug(modelLog) << "Private::updateAllUpstreamStatus: updating leaf node" << node->name;
        q->updateUpstreamStatus(node);
        return;
    }
    for (BranchNode *child : std::as_const(node->children))
        updateAllUpstreamStatus(child);
}

QString BranchModel::toolTip(const QString &hash) const
{
    qCDebug(modelLog) << "toolTip() called: hash=" << hash;
    // Show the hash description excluding diff as toolTip
    const Result<QString> res = gitClient().synchronousLog(d->workingDirectory, {"-n1", hash},
                                                           RunFlags::SuppressCommandLogging);
    const QString result = res ? res.value() : res.error();
    qCDebug(modelLog) << "toolTip: result=" << result;
    return result;
}

} // Git::Internal
