// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "branchmodel.h"

#include "gitclient.h"
#include "gitconstants.h"
#include "gittr.h"

#include <QtTaskTree/QConditional>
#include <QtTaskTree/QSingleTaskTreeRunner>
#include <QtTaskTree/QThreadFunction>

#include <coreplugin/vcsmanager.h>

#include <projectexplorer/project.h>
#include <projectexplorer/projectmanager.h>

#include <utils/algorithm.h>
#include <utils/qtcprocess.h>
#include <utils/qtcassert.h>
#include <utils/stringutils.h>

#include <vcsbase/vcscommand.h>
#include <vcsbase/vcsoutputwindow.h>

#include <QDateTime>
#include <QFont>
#include <QLoggingCategory>
#include <QSet>

#include <memory>
#include <set>

using namespace QtTaskTree;
using namespace Utils;
using namespace VcsBase;

namespace Git::Internal {

static Q_LOGGING_CATEGORY(nodeLog, "qtc.vcs.git.branches.node", QtWarningMsg);
static Q_LOGGING_CATEGORY(modelLog, "qtc.vcs.git.branches.model", QtWarningMsg);

static const Key checkoutRefKey("Git.LastCheckoutRef");

static QList<ProjectExplorer::Project *> projectsForRepository(const FilePath &repository)
{
    return Utils::filtered(ProjectExplorer::ProjectManager::projects(), [repository](
                               const ProjectExplorer::Project *project) {
        return Core::VcsManager::findTopLevelForDirectory(project->projectDirectory()) == repository;
    });
}

static void rememberCheckoutInProjects(const QList<ProjectExplorer::Project *> &projects,
                                       const FilePath &repository, const QStringList &checkout)
{
    const QVariant value = checkout.isEmpty()
                               ? QVariant{}
                               : QVariant(QStringList{repository.toUrlishString()} + checkout);
    for (ProjectExplorer::Project *project : projects)
        project->setNamedSettings(checkoutRefKey, value);
}

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
    int modified = 0;
    mutable QString toolTip;
    mutable bool toolTipFetchPending = false;
};

class BranchModel::Private
{
public:
    struct RefreshData
    {
        FilePath workingDirectory;
        QString currentHash;
        QString currentBranch;
        QDateTime currentDateTime;
        FilePath gitDir;
        QString refsOutput;
        bool refsSucceeded = false;
    };

    explicit Private(BranchModel *q) :
        q(q),
        rootNode(new BranchNode)
    {
    }

    Private(const Private &) = delete;
    Private &operator=(const Private &) = delete;

    ~Private()
    {
        taskTreeRunner.reset();
        parallelTaskTreeRunner.reset();
        delete rootNode;
    }

    bool hasTags() const { return rootNode->children.count() > Tags; }
    void parseOutputLine(const QString &line, bool force = false);
    void flushOldEntries();
    ExecutableItem updateUpstreamStatusTask(BranchNode *node);
    void updateAllUpstreamStatus(BranchNode *node);
    int currentBranchModifiedFiles();

    QStringList lastCheckout(const FilePath &repository) const
    {
        const auto projects = projectsForRepository(repository);
        if (projects.isEmpty())
            return checkoutRefs.value(repository);
        for (ProjectExplorer::Project *project : projects) {
            const QStringList checkout = project->namedSettings(checkoutRefKey).toStringList();
            if (checkout.size() == 3 && checkout.first() == repository.toUrlishString())
                return checkout.mid(1);
        }
        return {};
    }

    void rememberCheckout(const FilePath &repository, const QStringList &checkout)
    {
        const auto projects = projectsForRepository(repository);
        if (projects.isEmpty() && !checkout.isEmpty())
            checkoutRefs.insert(repository, checkout);
        else
            checkoutRefs.remove(repository);
        rememberCheckoutInProjects(projects, repository, checkout);
    }

    BranchModel *q = nullptr;
    FilePath workingDirectory;
    BranchNode *rootNode = nullptr;
    BranchNode *currentBranch = nullptr;
    BranchNode *headNode = nullptr;
    QHash<FilePath, QStringList> checkoutRefs;
    QString checkedOutRef;
    FilePath revealedRepository;
    QSet<QString> revealedRefs;
    BranchNode *checkedOutBranch = nullptr;
    QString currentHash;
    QDateTime currentDateTime;
    QStringList obsoleteLocalBranches;
    QSingleTaskTreeRunner taskTreeRunner;
    QParallelTaskTreeRunner parallelTaskTreeRunner;
    quint64 refreshGeneration = 0;
    bool oldBranchesIncluded = false;
    int refreshBlockCount = 0;
    bool refreshPending = false;
    RefreshMode pendingRefreshMode = RefreshMode::IfChanged;
    ShowError pendingShowError = ShowError::No;
    FilePath pendingRepository;
    bool refsLoaded = false;
    std::function<void()> deferredRefreshCompletion;
    bool checkoutRefreshEnabled = true;

    void queueRefresh(const FilePath &repository, ShowError showError, RefreshMode mode)
    {
        refreshPending = true;
        pendingRepository = repository;
        if (mode == RefreshMode::Force)
            pendingRefreshMode = RefreshMode::Force;
        if (showError == ShowError::Yes)
            pendingShowError = ShowError::Yes;
    }

    FilePath requestedRepository() const
    {
        return refreshBlockCount && refreshPending ? pendingRepository : workingDirectory;
    }

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

    connect(Core::VcsManager::instance(), &Core::VcsManager::repositoryChanged,
            this, &BranchModel::updateCurrentBranchModifiedFiles);
    connect(Core::VcsManager::instance(), &Core::VcsManager::updateFileState,
            this, &BranchModel::updateCurrentBranchModifiedFiles);
    connect(Core::VcsManager::instance(), &Core::VcsManager::clearFileState,
            this, &BranchModel::updateCurrentBranchModifiedFiles);
}

BranchModel::~BranchModel()
{
    qCInfo(modelLog) << "BranchModel destructed";
    delete d;
}

QModelIndex BranchModel::index(int row, int column, const QModelIndex &parentIdx) const
{
    qCDebug(modelLog) << "index() called: row=" << row << "column=" << column << "parentIdx=" << parentIdx;
    if (row < 0 || column < 0 || column > 1
        || (parentIdx.isValid() && parentIdx.column() != ColumnBranch)) {
        return {};
    }
    BranchNode *parentNode = parentIdx.isValid() ? indexToNode(parentIdx) : d->rootNode;
    if (!parentNode)
        return {};

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
    if (!node)
        return {};
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

    const BranchNode *node = parentIdx.isValid() ? indexToNode(parentIdx) : d->rootNode;
    if (!node)
        return 0;

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
    if (!index.isValid())
        return {};
    const QChar arrowUp(0x2191);
    const QChar arrowDown(0x2193);
    const QChar plusMinus(0x00B1);

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
            if (!node->isLeaf())
                break;

            if (!node->isLocal()) {
                if (node->modified)
                    res += " +" + QString::number(node->modified);
                break;
            }

            if (node->status.ahead >= 0)
                res += ' ' + arrowUp + QString::number(node->status.ahead);
            if (node->status.behind >= 0)
                res += ' ' + arrowDown + QString::number(node->status.behind);
            if (node->modified)
                res += ' ' + plusMinus + QString::number(node->modified);
            if (!node->tracking.isEmpty())
                res += " [" + node->tracking + ']';
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
        if (node->toolTip.isEmpty() && !node->toolTipFetchPending && !node->hash.isEmpty()) {
            node->toolTipFetchPending = true;
            const FilePath repository = d->workingDirectory;
            const QPointer<BranchNode> nodePtr = node;
            const QPointer<BranchModel> modelPtr(const_cast<BranchModel *>(this));
            const auto commandHandler = [repository, nodePtr, modelPtr](const CommandResult &result) {
                if (!nodePtr)
                    return;
                nodePtr->toolTipFetchPending = false;
                nodePtr->toolTip = result.result() == ProcessResult::FinishedWithSuccess
                                       ? result.cleanedStdOut() : result.cleanedStdErr();
                if (nodePtr->toolTip.isEmpty())
                    nodePtr->toolTip = nodePtr->fullRef();
                if (!modelPtr || modelPtr->d->workingDirectory != repository)
                    return;
                const QModelIndex idx = modelPtr->nodeToIndex(nodePtr.get(), ColumnBranch);
                if (idx.isValid())
                    emit modelPtr->dataChanged(idx, idx, {Qt::ToolTipRole});
            };
            const ExecutableItem task = gitClient().commandTask({
                .workingDirectory = repository,
                .arguments = {"log", "--no-color", "-n1", node->hash},
                .flags = RunFlag::SuppressCommandLogging,
                .commandHandler = commandHandler,
            });
            const_cast<BranchModel *>(this)->d->parallelTaskTreeRunner.start({task});
        }
        qCDebug(modelLog) << "data: ToolTipRole for node:" << node->name << "toolTip:" << node->toolTip;
        return node->toolTip.isEmpty() ? node->fullRef() : node->toolTip;
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
    if (!index.isValid())
        return Qt::NoItemFlags;
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
    d->refsLoaded = false;
    for (BranchNode *root : std::as_const(d->rootNode->children)) {
        QTC_ASSERT(root, continue);

        while (root->count()) {
            qCDebug(modelLog) << "clear: deleting child node:" << root->children.last()->name;
            delete root->children.takeLast();
        }
    }
    if (d->hasTags()) {
        qCDebug(modelLog) << "clear: removing tags node";
        delete d->rootNode->children.takeLast();
    }

    d->currentHash.clear();
    d->currentDateTime = {};
    d->currentBranch = nullptr;
    d->headNode = nullptr;
    d->checkedOutRef.clear();
    d->checkedOutBranch = nullptr;
    d->obsoleteLocalBranches.clear();
    d->currentRoot = nullptr;
    d->oldEntries.clear();
    qCDebug(modelLog) << "clear: model state reset";
}

void BranchModel::refresh(const FilePath &workingDirectory, ShowError showError,
                          RefreshMode mode)
{
    if (d->refreshBlockCount) {
        const bool supersedesActiveRefresh = d->deferredRefreshCompletion
                                             || d->taskTreeRunner.isRunning();
        const RefreshMode queuedMode = supersedesActiveRefresh ? RefreshMode::Force : mode;
        d->queueRefresh(workingDirectory, showError, queuedMode);
        d->deferredRefreshCompletion = {};
        return;
    }
    if (d->workingDirectory == workingDirectory && mode == RefreshMode::IfChanged) {
        if (d->taskTreeRunner.isRunning())
            return;
        if (d->refsLoaded) {
            emit refreshFinished(true);
            return;
        }
    }
    d->deferredRefreshCompletion = {};
    const bool repositoryWasChanged = d->workingDirectory != workingDirectory;
    emit refreshStarted();
    const quint64 generation = ++d->refreshGeneration;
    d->taskTreeRunner.reset();

    if (workingDirectory.isEmpty()) {
        d->parallelTaskTreeRunner.reset();
        beginResetModel();
        clear();
        d->workingDirectory = {};
        d->refsLoaded = true;
        endResetModel();
        if (repositoryWasChanged)
            emit repositoryChanged();
        emit refreshFinished(true);
        return;
    }

    if (repositoryWasChanged) {
        d->parallelTaskTreeRunner.reset();
        beginResetModel();
        clear();
        d->workingDirectory = workingDirectory;
        d->refsLoaded = false;
        endResetModel();
        emit repositoryChanged();
    }

    const auto refreshData = std::make_shared<Private::RefreshData>();
    refreshData->workingDirectory = workingDirectory;

    const GroupItem topRevisionProc = gitClient().topRevision(workingDirectory,
        [refreshData](const QString &ref, const QDateTime &dateTime) {
            refreshData->currentHash = ref;
            refreshData->currentDateTime = dateTime;
        });

    const auto onForEachRefSetup = [workingDirectory](Process &process) {
        QStringList args = {"for-each-ref",
                            "--format=%(objectname)\t%(refname)\t%(upstream:short)\t"
                            "%(*objectname)\t%(committerdate:raw)\t%(*committerdate:raw)",
                            "refs/heads/**",
                            "refs/remotes/**"};
        if (settings().showTags())
            args << "refs/tags/**";
        gitClient().setupCommand(process, workingDirectory, args);
    };

    const auto onForEachRefDone = [refreshData, workingDirectory, showError](
                                       const Process &process, DoneWith result) {
        if (result != DoneWith::Success) {
            if (showError == ShowError::No)
                return;
            const QString message = Tr::tr("Cannot run \"%1\" in \"%2\": %3")
                                        .arg("git for-each-ref")
                                        .arg(workingDirectory.toUserOutput())
                                        .arg(process.cleanedStdErr());
            VcsBase::VcsOutputWindow::appendError(workingDirectory, message);
            return;
        }
        refreshData->refsOutput = process.stdOut();
        refreshData->refsSucceeded = true;
    };

    const auto onCurrentBranchSetup = [workingDirectory](Process &process) {
        gitClient().setupCommand(process, workingDirectory, {"symbolic-ref", "--quiet", "HEAD"});
    };
    const auto onGitDirSetup = [workingDirectory](Process &process) {
        gitClient().setupCommand(process, workingDirectory, {"rev-parse", "--git-dir"});
    };
    const auto onGitDirDone = [refreshData](const Process &process, DoneWith result) {
        if (result == DoneWith::Success) {
            const QString gitDir = process.cleanedStdOut().trimmed();
            if (!gitDir.isEmpty())
                refreshData->gitDir = refreshData->workingDirectory.resolvePath(gitDir);
        }
        return DoneResult::Success;
    };
    const auto onCurrentBranchDone = [refreshData](const Process &process, DoneWith result) {
        if (result == DoneWith::Success) {
            const QString branch = process.cleanedStdOut().trimmed();
            const QString refsHeadsPrefix = "refs/heads/";
            if (branch.startsWith(refsHeadsPrefix))
                refreshData->currentBranch = branch.mid(refsHeadsPrefix.size());
        }
        return DoneResult::Success;
    };
    const auto onRebaseHeadSetup = [refreshData](QThreadFunction<QString> &task) {
        const FilePath gitDir = refreshData->gitDir;
        task.setThreadFunctionData([gitDir] {
            if (gitDir.isEmpty())
                return QString{};
            for (const QString &rebaseDir : QStringList{"rebase-merge", "rebase-apply"}) {
                const Result<QByteArray> head = (gitDir / rebaseDir / "head-name").fileContents();
                if (head)
                    return QString::fromUtf8(*head).trimmed();
            }
            return QString{};
        });
    };
    const auto onRebaseHeadDone = [refreshData](const QThreadFunction<QString> &task) {
        if (!refreshData->currentBranch.isEmpty())
            return;

        const QString branch = task.result();
        const QString refsHeadsPrefix = "refs/heads/";
        if (branch.startsWith(refsHeadsPrefix))
            refreshData->currentBranch = branch.mid(refsHeadsPrefix.size());
    };

    const Group recipe {
        topRevisionProc,
        ProcessTask(onForEachRefSetup, onForEachRefDone),
        ProcessTask(onCurrentBranchSetup, onCurrentBranchDone),
        If ([refreshData] { return refreshData->currentBranch.isEmpty(); }) >> Then {
            ProcessTask(onGitDirSetup, onGitDirDone),
            QThreadFunctionTask<QString>(onRebaseHeadSetup, onRebaseHeadDone)
        }
    };
    d->taskTreeRunner.start(recipe, {}, [this, refreshData, generation] {
        if (generation != d->refreshGeneration)
            return;

        emit refreshResultReady(refreshData->refsSucceeded);
        const auto applyResult = [this, refreshData, generation] {
            if (generation != d->refreshGeneration)
                return;

            d->parallelTaskTreeRunner.reset();
            beginResetModel();
            clear();
            d->workingDirectory = refreshData->workingDirectory;
            d->currentHash = refreshData->currentHash;
            d->currentDateTime = refreshData->currentDateTime;

            if (refreshData->refsSucceeded) {
                const QStringList lastCheckout = d->lastCheckout(d->workingDirectory);
                if (refreshData->currentBranch.isEmpty() && lastCheckout.size() == 2
                    && lastCheckout.at(1) == d->currentHash) {
                    d->checkedOutRef = lastCheckout.first();
                } else if (!lastCheckout.isEmpty()) {
                    d->rememberCheckout(d->workingDirectory, {});
                }
                const QStringList lines = refreshData->refsOutput.split('\n');
                for (const QString &line : lines)
                    d->parseOutputLine(line);
                d->flushOldEntries();

                if (d->checkedOutBranch)
                    d->currentBranch = d->checkedOutBranch;
                if (d->currentBranch && d->currentBranch->isLocal())
                    d->currentBranch = nullptr;
                setCurrentBranch(refreshData->currentBranch);
                if (!d->currentBranch) {
                    BranchNode *local = d->rootNode->children.at(LocalBranches);
                    if (!local) {
                        endResetModel();
                        d->refsLoaded = false;
                        emit refreshFinished(false);
                        return;
                    }
                    d->currentBranch = d->headNode = new BranchNode(
                        Tr::tr("Detached HEAD"), "HEAD", {}, d->currentDateTime);
                    local->prepend(d->headNode);
                }
            }
            endResetModel();

            d->refsLoaded = refreshData->refsSucceeded;
            emit refreshFinished(refreshData->refsSucceeded);
            if (!refreshData->refsSucceeded)
                return;
            d->updateAllUpstreamStatus(d->rootNode->children.at(LocalBranches));
            updateCurrentBranchModifiedFiles();
        };

        if (d->refreshBlockCount) {
            d->deferredRefreshCompletion = applyResult;
            return;
        }
        applyResult();
    });
}

void BranchModel::setCurrentBranch(const QString &currentBranch)
{
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
    const FilePath repository = d->workingDirectory;
    const QPointer<BranchModel> self(this);
    gitClient().enqueueCommand({
        .workingDirectory = repository,
        .arguments = {"branch", "-m", oldName, newName},
        .commandHandler = [self, repository](const CommandResult &result) {
            if (result.result() != ProcessResult::FinishedWithSuccess) {
                VcsOutputWindow::appendError(repository, result.cleanedStdErr());
                return;
            }
            if (self && self->d->requestedRepository() == repository)
                self->refreshCurrentRepository();
        },
    });
}

void BranchModel::renameTag(const QString &oldName, const QString &newName)
{
    const FilePath repository = d->workingDirectory;
    const QPointer<BranchModel> self(this);
    const auto reportResult = [self, repository](const CommandResult &result) {
        if (result.result() != ProcessResult::FinishedWithSuccess)
            VcsOutputWindow::appendError(repository, result.cleanedStdErr());
        if (self && self->d->requestedRepository() == repository)
            self->refreshCurrentRepository();
    };
    gitClient().enqueueTask(Group{
        gitClient().commandTask({
            .workingDirectory = repository,
            .arguments = {"tag", newName, oldName},
            .commandHandler = reportResult,
        }),
        gitClient().commandTask({
            .workingDirectory = repository,
            .arguments = {"tag", "-d", oldName},
            .commandHandler = reportResult,
        }),
    });
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

QString BranchModel::tracking(const QModelIndex &idx) const
{
    if (!idx.isValid())
        return {};
    BranchNode *node = indexToNode(idx);
    if (!node || !node->isLeaf())
        return {};
    return node->tracking;
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
    const QString branch = fullName(idx);
    if (!isLeaf(idx) || branch.isEmpty())
        return;
    const FilePath repository = d->workingDirectory;
    const QPointer<BranchModel> self(this);
    gitClient().enqueueCommand({
        .workingDirectory = repository,
        .arguments = {"branch", "-D", branch},
        .commandHandler = [self, repository](const CommandResult &result) {
            if (result.result() != ProcessResult::FinishedWithSuccess) {
                VcsOutputWindow::appendError(repository, result.cleanedStdErr());
                return;
            }
            if (self && self->d->requestedRepository() == repository)
                self->refreshCurrentRepository();
        },
    });
}

void BranchModel::removeTag(const QModelIndex &idx)
{
    const QString tag = fullName(idx);
    if (!isLeaf(idx) || tag.isEmpty())
        return;
    const FilePath repository = d->workingDirectory;
    const QPointer<BranchModel> self(this);
    gitClient().enqueueCommand({
        .workingDirectory = repository,
        .arguments = {"tag", "-d", tag},
        .commandHandler = [self, repository](const CommandResult &result) {
            if (result.result() != ProcessResult::FinishedWithSuccess) {
                VcsOutputWindow::appendError(repository, result.cleanedStdErr());
                return;
            }
            if (self && self->d->requestedRepository() == repository)
                self->refreshCurrentRepository();
        },
    });
}

void BranchModel::checkoutBranch(const QModelIndex &idx,
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
    const FilePath workingDirectory = d->workingDirectory;
    const QPointer<BranchModel> guard(this);
    const auto onCheckoutDone = [workingDirectory, branch, guard, handler](
                                   const CommandResult &result) {
        if (result.result() == ProcessResult::FinishedWithSuccess) {
            QStringList checkout;
            if ((branch.startsWith("refs/remotes/") || branch.startsWith("refs/tags/"))
                && gitClient().synchronousCurrentLocalBranch(workingDirectory).isEmpty()) {
                QString commit;
                if (gitClient().synchronousRevParseCmd(workingDirectory, "HEAD", &commit))
                    checkout = {branch, commit.trimmed()};
            }
            if (guard)
                guard->d->rememberCheckout(workingDirectory, checkout);
            else
                rememberCheckoutInProjects(projectsForRepository(workingDirectory), workingDirectory,
                                           checkout);
        }
        if (result.result() == ProcessResult::FinishedWithSuccess && guard
            && guard->d->requestedRepository() == workingDirectory
            && guard->d->checkoutRefreshEnabled) {
            guard->refreshCurrentRepository();
        }
        if (handler)
            handler(result);
    };
    gitClient().checkout(workingDirectory, branch, GitClient::StashMode::NoStash, onCheckoutDone);
}

void BranchModel::branchIsMerged(const QModelIndex &idx,
                                 const std::function<void(bool)> &handler)
{
    qCDebug(modelLog) << "branchIsMerged() called: idx=" << idx;
    const QString branch = fullName(idx, true);
    const QString branchHash = hash(idx);
    if (branch.isEmpty() || branchHash.isEmpty()) {
        qCWarning(modelLog) << "branchIsMerged: branch name is empty for idx=" << idx;
        if (handler)
            handler(false);
        return;
    }

    const FilePath repository = d->workingDirectory;
    const QPointer<BranchModel> self(this);
    const auto commandHandler = [self, repository, branch, handler](const CommandResult &result) {
        if (!self)
            return;
        if (self->d->requestedRepository() != repository) {
            if (handler)
                handler(false);
            return;
        }
        if (result.result() != ProcessResult::FinishedWithSuccess) {
            qCWarning(modelLog) << "branchIsMerged: git branch contains failed:"
                                << result.cleanedStdErr();
            VcsOutputWindow::appendError(repository, result.cleanedStdErr());
            if (handler)
                handler(false);
            return;
        }

        const QStringList lines = result.cleanedStdOut().split('\n', Qt::SkipEmptyParts);
        const bool merged = std::any_of(lines.cbegin(), lines.cend(), [branch](const QString &line) {
            return line != branch;
        });
        qCDebug(modelLog) << "branchIsMerged:" << branch << "merged:" << merged;
        if (handler)
            handler(merged);
    };
    gitClient().enqueueCommand({
        .workingDirectory = repository,
        .arguments = {"for-each-ref", "--format=%(refname)", "--contains=" + branchHash,
                      "refs/heads/", "refs/remotes/"},
        .commandHandler = commandHandler,
    });
}

void BranchModel::addBranch(const QString &name, bool track, const QModelIndex &startPoint,
                            const std::function<void(const QModelIndex &)> &handler)
{
    qCDebug(modelLog) << "addBranch() called: name=" << name << "track=" << track << "startPoint=" << startPoint;
    if (!d->rootNode || !d->rootNode->count()) {
        qCWarning(modelLog) << "addBranch: no root node or no children";
        if (handler)
            handler({});
        return;
    }

    const FilePath workingDirectory = d->workingDirectory;
    const QString fullTrackedBranch = fullName(startPoint, true);
    QStringList args = {"branch", track ? QString("--track") : QString("--no-track"), name};
    args << (fullTrackedBranch.isEmpty() ? QString("HEAD") : fullTrackedBranch);
    const QPointer<BranchModel> self(this);
    const auto commandHandler = [self, workingDirectory, name, handler](const CommandResult &result) {
        if (!self)
            return;
        if (result.result() != ProcessResult::FinishedWithSuccess) {
            qCWarning(modelLog) << "addBranch: git branch creation failed:" << result.cleanedStdErr();
            VcsOutputWindow::appendError(workingDirectory, result.cleanedStdErr());
            if (handler)
                handler({});
            return;
        }
        if (self->d->requestedRepository() != workingDirectory) {
            if (handler)
                handler({});
            return;
        }

        if (self->d->revealedRepository != workingDirectory) {
            self->d->revealedRepository = workingDirectory;
            self->d->revealedRefs.clear();
        }
        self->d->revealedRefs.insert("refs/heads/" + name);
        auto *context = new QObject(self);
        const auto complete = [self, context, handler](const QModelIndex &index) {
            QObject::disconnect(self, nullptr, context, nullptr);
            context->deleteLater();
            if (handler)
                handler(index);
        };
        QObject::connect(self, &BranchModel::repositoryChanged, context,
                         [self, workingDirectory, complete] {
            if (self->d->requestedRepository() != workingDirectory)
                complete({});
        });
        QObject::connect(self, &BranchModel::refreshFinished, context,
                         [self, workingDirectory, name, complete](bool success) {
            if (!success || self->d->requestedRepository() != workingDirectory) {
                complete({});
                return;
            }
            const auto findBranch = [&name](BranchNode *node, const auto &find) -> BranchNode * {
                if (node->isLeaf() && node->fullRef() == name)
                    return node;
                for (BranchNode *child : std::as_const(node->children)) {
                    if (BranchNode *found = find(child, find))
                        return found;
                }
                return nullptr;
            };
            BranchNode *node = findBranch(self->d->rootNode->children.at(LocalBranches), findBranch);
            complete(node && node->isLeaf() ? self->nodeToIndex(node, ColumnBranch)
                                           : QModelIndex{});
        });
        self->refresh(workingDirectory, ShowError::Yes, RefreshMode::Force);
    };
    gitClient().enqueueCommand({
        .workingDirectory = workingDirectory,
        .arguments = args,
        .commandHandler = commandHandler,
    });
}

void BranchModel::setRemoteTracking(const QModelIndex &trackingIndex)
{
    const QModelIndex current = currentBranch();
    QTC_ASSERT(current.isValid(), return);
    const QString currentName = fullName(current);
    const QString tracking = fullName(trackingIndex, true);
    const FilePath repository = d->workingDirectory;
    const QStringList arguments = trackingIndex.isValid()
        ? QStringList{"branch", "--set-upstream-to=" + tracking, currentName}
        : QStringList{"branch", "--unset-upstream", currentName};
    const QPointer<BranchModel> self(this);
    gitClient().enqueueCommand({
        .workingDirectory = repository,
        .arguments = arguments,
        .commandHandler = [self, repository](const CommandResult &result) {
            if (result.result() != ProcessResult::FinishedWithSuccess) {
                VcsOutputWindow::appendError(repository, result.cleanedStdErr());
                return;
            }
            if (self && self->d->requestedRepository() == repository)
                self->refreshCurrentRepository();
        },
    });
}

void BranchModel::setOldBranchesIncluded(bool value)
{
    if (d->oldBranchesIncluded == value)
        return;
    qCDebug(modelLog) << "setOldBranchesIncluded() called: value=" << value;
    d->oldBranchesIncluded = value;
    emit oldBranchesIncludedChanged(value);
    refreshCurrentRepository();
}

bool BranchModel::oldBranchesIncluded() const
{
    return d->oldBranchesIncluded;
}

void BranchModel::setTagsIncluded(bool value)
{
    if (settings().showTags() == value)
        return;
    settings().showTags.setValue(value);
    emit tagsIncludedChanged(value);
    refreshCurrentRepository();
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

    if (node->isLocal())
        d->parallelTaskTreeRunner.start({d->updateUpstreamStatusTask(node)});
    else
        updateCurrentBranchModifiedFiles();
    qCDebug(modelLog) << "refreshCurrentBranch: upstream status updated for" << node->name;
}

void BranchModel::updateCurrentBranchModifiedFiles()
{
    BranchNode *node = d->currentBranch;
    if (!node)
        return;

    node->modified = d->currentBranchModifiedFiles();
    const QModelIndex idx = nodeToIndex(node, ColumnBranch);
    emit dataChanged(idx, idx, {Qt::DisplayRole});
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
    if (!oldBranchesIncluded && !force && !current && dateTime.isValid()
        && !(revealedRepository == workingDirectory && revealedRefs.contains(fullName))) {
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
        if (fullName == checkedOutRef)
            checkedOutBranch = newNode;
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
    if (!index.isValid())
        return nullptr;
    if (index.model() != this)
        return nullptr;
    if (index.column() < 0 || index.column() > 1)
        return nullptr;
    return static_cast<BranchNode *>(index.internalPointer());
}

QModelIndex BranchModel::nodeToIndex(BranchNode *node, int column) const
{
    QTC_ASSERT(node, return {});
    QTC_ASSERT(node->parent, return {});
    qCDebug(modelLog) << "nodeToIndex() called: node=" << node->name << "column=" << column;
    if (node == d->rootNode)
        return {};
    const int row = node->parent->rowOf(node);
    if (row < 0)
        return {};
    const QModelIndex idx = createIndex(row, column, static_cast<void *>(node));
    qCDebug(modelLog) << "nodeToIndex: returning index" << idx;
    return idx;
}

ExecutableItem BranchModel::Private::updateUpstreamStatusTask(BranchNode *node)
{
    if (!node || !node->isLocal())
        return successItem;

    QStringList arguments = {"rev-list", "--no-color", "--count"};
    const QString fullRef = (node == headNode) ? "HEAD" : node->fullRef();
    if (node->tracking.isEmpty())
        arguments += {fullRef, "--not", "--remotes"};
    else
        arguments += {"--left-right", fullRef + "..." + node->tracking, "--"};

    const auto commandHandler = [this, nodePtr = QPointer<BranchNode>(node)](const CommandResult &result) {
        if (!nodePtr)
            return;

        qCDebug(modelLog) << "updateUpstreamStatus: process done for node" << nodePtr->name
                          << "result=" << int(result.result());
        if (result.result() != ProcessResult::FinishedWithSuccess)
            return;
        const QString text = result.cleanedStdOut();
        if (text.isEmpty())
            return;
        const QStringList split = text.trimmed().split('\t');
        if (nodePtr->tracking.isEmpty()) {
            nodePtr->setUpstreamStatus(UpstreamStatus(split.at(0).toInt(), 0));
        } else {
            QTC_ASSERT(split.size() == 2, return);

            nodePtr->setUpstreamStatus(UpstreamStatus(split.at(0).toInt(), split.at(1).toInt()));
        }

        if (nodePtr.get() == currentBranch)
            nodePtr->modified = currentBranchModifiedFiles();

        const QModelIndex idx = q->nodeToIndex(nodePtr.get(), ColumnBranch);
        if (idx.isValid()) {
            emit q->dataChanged(idx, idx, {Qt::DisplayRole});
            qCDebug(modelLog) << "updateUpstreamStatus: dataChanged emitted for node" << nodePtr->name;
        }
    };

    return gitClient().commandTask({
        .workingDirectory = workingDirectory,
        .arguments = arguments,
        .flags = RunFlag::SuppressCommandLogging,
        .commandHandler = commandHandler,
    });
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
        parallelTaskTreeRunner.start({updateUpstreamStatusTask(node)});
        return;
    }
    for (BranchNode *child : std::as_const(node->children))
        updateAllUpstreamStatus(child);
}

int BranchModel::Private::currentBranchModifiedFiles()
{
    return Core::VcsManager::instance()->modifiedFiles(workingDirectory).size();
}

Utils::FilePath BranchModel::repository() const
{
    return d->workingDirectory;
}

void BranchModel::setRepository(const FilePath &repository)
{
    refresh(repository, ShowError::Yes, RefreshMode::IfChanged);
}

void BranchModel::refreshIfSame(const FilePath &repository)
{
    if (d->requestedRepository() != repository)
        return;
    refreshCurrentRepository();
}

void BranchModel::refreshCurrentRepository()
{
    const FilePath repository = d->requestedRepository();
    refresh(repository, ShowError::Yes);
}

void BranchModel::setRefreshBlocked(bool blocked)
{
    if (blocked) {
        ++d->refreshBlockCount;
        return;
    }
    QTC_ASSERT(d->refreshBlockCount > 0, return);
    if (--d->refreshBlockCount == 0 && d->refreshPending) {
        const FilePath repository = d->pendingRepository;
        const ShowError showError = d->pendingShowError;
        const RefreshMode mode = d->pendingRefreshMode;
        d->refreshPending = false;
        d->pendingRepository = {};
        d->pendingShowError = ShowError::No;
        d->pendingRefreshMode = RefreshMode::IfChanged;
        d->deferredRefreshCompletion = {};
        refresh(repository, showError, mode);
    } else if (!d->refreshBlockCount && d->deferredRefreshCompletion) {
        std::function<void()> completion = std::move(d->deferredRefreshCompletion);
        d->deferredRefreshCompletion = {};
        completion();
    }
}

void BranchModel::setCheckoutRefreshEnabled(bool enabled)
{
    d->checkoutRefreshEnabled = enabled;
}

bool BranchModel::checkoutRefreshEnabled() const
{
    return d->checkoutRefreshEnabled;
}

} // Git::Internal
