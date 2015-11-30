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

#include "branchmodel.h"
#include "gitclient.h"

#include <utils/qtcassert.h>
#include <vcsbase/vcsoutputwindow.h>
#include <vcsbase/vcscommand.h>

#include <QFont>

using namespace VcsBase;

namespace Git {
namespace Internal {

enum RootNodes {
    LocalBranches = 0,
    RemoteBranches = 1,
    Tags = 2
};

// --------------------------------------------------------------------------
// BranchNode:
// --------------------------------------------------------------------------

class BranchNode
{
public:
    BranchNode() :
        parent(0),
        name(QLatin1String("<ROOT>"))
    { }

    BranchNode(const QString &n, const QString &s = QString(), const QString &t = QString()) :
        parent(0), name(n), sha(s), tracking(t)
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
        return 0;
    }

    QStringList fullName(bool includePrefix = false) const
    {
        QTC_ASSERT(isLeaf(), return QStringList());

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

        foreach (const BranchNode *n, nodes)
            fn.append(n->name);

        return fn;
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

    QStringList childrenNames() const
    {
        if (children.count() > 0) {
            QStringList names;
            foreach (BranchNode *n, children) {
                names.append(n->childrenNames());
            }
            return names;
        }
        return QStringList(fullName().join(QLatin1Char('/')));
    }

    int rowOf(BranchNode *node)
    {
        return children.indexOf(node);
    }

    BranchNode *parent;
    QList<BranchNode *> children;

    QString name;
    QString sha;
    QString tracking;
    mutable QString toolTip;
};

// --------------------------------------------------------------------------
// BranchModel:
// --------------------------------------------------------------------------

BranchModel::BranchModel(GitClient *client, QObject *parent) :
    QAbstractItemModel(parent),
    m_client(client),
    m_rootNode(new BranchNode),
    m_currentBranch(0)
{
    QTC_CHECK(m_client);

    // Abuse the sha field for ref prefix
    m_rootNode->append(new BranchNode(tr("Local Branches"), QLatin1String("refs/heads")));
    m_rootNode->append(new BranchNode(tr("Remote Branches"), QLatin1String("refs/remotes")));
}

BranchModel::~BranchModel()
{
    delete m_rootNode;
}

QModelIndex BranchModel::index(int row, int column, const QModelIndex &parentIdx) const
{
    if (column != 0)
        return QModelIndex();
    BranchNode *parentNode = indexToNode(parentIdx);

    if (row >= parentNode->count())
        return QModelIndex();
    return nodeToIndex(parentNode->children.at(row));
}

QModelIndex BranchModel::parent(const QModelIndex &index) const
{
    if (!index.isValid())
        return QModelIndex();

    BranchNode *node = indexToNode(index);
    if (node->parent == m_rootNode)
        return QModelIndex();
    return nodeToIndex(node->parent);
}

int BranchModel::rowCount(const QModelIndex &parentIdx) const
{
    if (parentIdx.column() > 0)
        return 0;

    return indexToNode(parentIdx)->count();
}

int BranchModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 1;
}

QVariant BranchModel::data(const QModelIndex &index, int role) const
{
    BranchNode *node = indexToNode(index);
    if (!node)
        return QVariant();

    switch (role) {
    case Qt::DisplayRole: {
        QString res = node->name;
        if (!node->tracking.isEmpty())
            res += QLatin1String(" [") + node->tracking + QLatin1Char(']');
        return res;
    }
    case Qt::EditRole:
        return node->name;
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
        } else if (node == m_currentBranch) {
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
    if (role != Qt::EditRole)
        return false;
    BranchNode *node = indexToNode(index);
    if (!node)
        return false;

    const QString newName = value.toString();
    if (newName.isEmpty())
        return false;

    if (node->name == newName)
        return true;

    QStringList oldFullName = node->fullName();
    node->name = newName;
    QStringList newFullName = node->fullName();

    QString output;
    QString errorMessage;
    if (!m_client->synchronousBranchCmd(m_workingDirectory,
                                        QStringList() << QLatin1String("-m")
                                                      << oldFullName.last()
                                                      << newFullName.last(),
                                        &output, &errorMessage)) {
        node->name = oldFullName.last();
        VcsOutputWindow::appendError(errorMessage);
        return false;
    }

    emit dataChanged(index, index);
    return true;
}

Qt::ItemFlags BranchModel::flags(const QModelIndex &index) const
{
    BranchNode *node = indexToNode(index);
    if (!node)
        return Qt::NoItemFlags;
    if (node->isLeaf() && node->isLocal())
        return Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsEnabled;
    else
        return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
}

void BranchModel::clear()
{
    foreach (BranchNode *root, m_rootNode->children)
        while (root->count())
            delete root->children.takeLast();
    if (hasTags())
        m_rootNode->children.takeLast();

    m_currentBranch = 0;
}

bool BranchModel::refresh(const QString &workingDirectory, QString *errorMessage)
{
    beginResetModel();
    clear();
    if (workingDirectory.isEmpty()) {
        endResetModel();
        return false;
    }

    m_currentSha = m_client->synchronousTopRevision(workingDirectory);
    QStringList args;
    args << QLatin1String("--format=%(objectname)\t%(refname)\t%(upstream:short)\t%(*objectname)");
    QString output;
    if (!m_client->synchronousForEachRefCmd(workingDirectory, args, &output, errorMessage))
        VcsOutputWindow::appendError(*errorMessage);

    m_workingDirectory = workingDirectory;
    const QStringList lines = output.split(QLatin1Char('\n'));
    foreach (const QString &l, lines)
        parseOutputLine(l);

    if (m_currentBranch) {
        if (m_currentBranch->parent == m_rootNode->children.at(LocalBranches))
            m_currentBranch = 0;
        setCurrentBranch();
    }

    endResetModel();

    return true;
}

void BranchModel::setCurrentBranch()
{
    QString currentBranch = m_client->synchronousCurrentLocalBranch(m_workingDirectory);
    if (currentBranch.isEmpty())
        return;

    BranchNode *local = m_rootNode->children.at(LocalBranches);
    int pos = 0;
    for (pos = 0; pos < local->count(); ++pos) {
        if (local->children.at(pos)->name == currentBranch)
            m_currentBranch = local->children[pos];
    }
}

void BranchModel::renameBranch(const QString &oldName, const QString &newName)
{
    QString errorMessage;
    QString output;
    if (!m_client->synchronousBranchCmd(m_workingDirectory,
                                        QStringList() << QLatin1String("-m") << oldName << newName,
                                        &output, &errorMessage))
        VcsOutputWindow::appendError(errorMessage);
    else
        refresh(m_workingDirectory, &errorMessage);
}

void BranchModel::renameTag(const QString &oldName, const QString &newName)
{
    QString errorMessage;
    QString output;
    if (!m_client->synchronousTagCmd(m_workingDirectory, QStringList() << newName << oldName,
                                     &output, &errorMessage)
     || !m_client->synchronousTagCmd(m_workingDirectory,
                                     QStringList() << QLatin1String("-d") << oldName,
                                     &output, &errorMessage)) {
        VcsOutputWindow::appendError(errorMessage);
    } else {
        refresh(m_workingDirectory, &errorMessage);
    }
}

QString BranchModel::workingDirectory() const
{
    return m_workingDirectory;
}

GitClient *BranchModel::client() const
{
    return m_client;
}

QModelIndex BranchModel::currentBranch() const
{
    if (!m_currentBranch)
        return QModelIndex();
    return nodeToIndex(m_currentBranch);
}

QString BranchModel::fullName(const QModelIndex &idx, bool includePrefix) const
{
    if (!idx.isValid())
        return QString();
    BranchNode *node = indexToNode(idx);
    if (!node || !node->isLeaf())
        return QString();
    QStringList path = node->fullName(includePrefix);
    return path.join(QLatin1Char('/'));
}

QStringList BranchModel::localBranchNames() const
{
    if (!m_rootNode || !m_rootNode->count())
        return QStringList();

    return m_rootNode->children.at(LocalBranches)->childrenNames();
}

QString BranchModel::sha(const QModelIndex &idx) const
{
    if (!idx.isValid())
        return QString();
    BranchNode *node = indexToNode(idx);
    return node->sha;
}

bool BranchModel::hasTags() const
{
    return m_rootNode->children.count() > Tags;
}

bool BranchModel::isLocal(const QModelIndex &idx) const
{
    if (!idx.isValid())
        return false;
    BranchNode *node = indexToNode(idx);
    return node->isLocal();
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
    if (!idx.isValid() || !hasTags())
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
    QStringList args;

    args << QLatin1String("-D") << branch;
    if (!m_client->synchronousBranchCmd(m_workingDirectory, args, &output, &errorMessage)) {
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
    QStringList args;

    args << QLatin1String("-d") << tag;
    if (!m_client->synchronousTagCmd(m_workingDirectory, args, &output, &errorMessage)) {
        VcsOutputWindow::appendError(errorMessage);
        return;
    }
    removeNode(idx);
}

void BranchModel::checkoutBranch(const QModelIndex &idx)
{
    QString branch = fullName(idx, !isLocal(idx));
    if (branch.isEmpty())
        return;

    // No StashGuard since this function for now is only used with clean working dir.
    // If it is ever used from another place, please add StashGuard here
    m_client->synchronousCheckout(m_workingDirectory, branch);
}

bool BranchModel::branchIsMerged(const QModelIndex &idx)
{
    QString branch = fullName(idx);
    if (branch.isEmpty())
        return false;

    QString errorMessage;
    QString output;
    QStringList args;

    args << QLatin1String("-a") << QLatin1String("--contains") << sha(idx);
    if (!m_client->synchronousBranchCmd(m_workingDirectory, args, &output, &errorMessage))
        VcsOutputWindow::appendError(errorMessage);

    QStringList lines = output.split(QLatin1Char('\n'), QString::SkipEmptyParts);
    foreach (const QString &l, lines) {
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
    if (!m_rootNode || !m_rootNode->count())
        return QModelIndex();

    const QString trackedBranch = fullName(startPoint);
    const QString fullTrackedBranch = fullName(startPoint, true);
    QString startSha;
    QString output;
    QString errorMessage;

    QStringList args;
    args << (track ? QLatin1String("--track") : QLatin1String("--no-track"));
    args << name;
    if (!fullTrackedBranch.isEmpty()) {
        args << fullTrackedBranch;
        startSha = sha(startPoint);
    } else {
        startSha = m_client->synchronousTopRevision(m_workingDirectory);
    }

    if (!m_client->synchronousBranchCmd(m_workingDirectory, args, &output, &errorMessage)) {
        VcsOutputWindow::appendError(errorMessage);
        return QModelIndex();
    }

    BranchNode *local = m_rootNode->children.at(LocalBranches);
    const int slash = name.indexOf(QLatin1Char('/'));
    const QString leafName = slash == -1 ? name : name.mid(slash + 1);
    bool added = false;
    if (slash != -1) {
        const QString nodeName = name.left(slash);
        int pos = positionForName(local, nodeName);
        BranchNode *child = (pos == local->count()) ? 0 : local->children.at(pos);
        if (!child || child->name != nodeName) {
            child = new BranchNode(nodeName);
            beginInsertRows(nodeToIndex(local), pos, pos);
            added = true;
            child->parent = local;
            local->children.insert(pos, child);
        }
        local = child;
    }
    int pos = positionForName(local, leafName);
    auto newNode = new BranchNode(leafName, startSha, track ? trackedBranch : QString());
    if (!added)
        beginInsertRows(nodeToIndex(local), pos, pos);
    newNode->parent = local;
    local->children.insert(pos, newNode);
    endInsertRows();
    return nodeToIndex(newNode);
}

void BranchModel::setRemoteTracking(const QModelIndex &trackingIndex)
{
    QModelIndex current = currentBranch();
    QTC_ASSERT(current.isValid(), return);
    const QString currentName = fullName(current);
    const QString shortTracking = fullName(trackingIndex);
    const QString tracking = fullName(trackingIndex, true);
    m_client->synchronousSetTrackingBranch(m_workingDirectory, currentName, tracking);
    m_currentBranch->tracking = shortTracking;
    emit dataChanged(current, current);
}

void BranchModel::parseOutputLine(const QString &line)
{
    if (line.size() < 3)
        return;

    QStringList lineParts = line.split(QLatin1Char('\t'));
    const QString shaDeref = lineParts.at(3);
    const QString sha = shaDeref.isEmpty() ? lineParts.at(0) : shaDeref;
    const QString fullName = lineParts.at(1);

    bool current = (sha == m_currentSha);
    bool showTags = m_client->settings().boolValue(GitSettings::showTagsKey);

    // insert node into tree:
    QStringList nameParts = fullName.split(QLatin1Char('/'));
    nameParts.removeFirst(); // remove refs...

    BranchNode *root = 0;
    if (nameParts.first() == QLatin1String("heads")) {
        root = m_rootNode->children.at(LocalBranches);
    } else if (nameParts.first() == QLatin1String("remotes")) {
        root = m_rootNode->children.at(RemoteBranches);
    } else if (showTags && nameParts.first() == QLatin1String("tags")) {
        if (!hasTags()) // Tags is missing, add it
            m_rootNode->append(new BranchNode(tr("Tags"), QLatin1String("refs/tags")));
        root = m_rootNode->children.at(Tags);
    } else {
        return;
    }

    nameParts.removeFirst();

    // limit depth of list. Git basically only ever wants one / and considers the rest as part of
    // the name.
    while (nameParts.count() > 3) {
        nameParts[2] = nameParts.at(2) + QLatin1Char('/') + nameParts.at(3);
        nameParts.removeAt(3);
    }

    const QString name = nameParts.last();
    nameParts.removeLast();

    auto newNode = new BranchNode(name, sha, lineParts.at(2));
    root->insert(nameParts, newNode);
    if (current)
        m_currentBranch = newNode;
}

BranchNode *BranchModel::indexToNode(const QModelIndex &index) const
{
    if (index.column() > 0)
        return 0;
    if (!index.isValid())
        return m_rootNode;
    return static_cast<BranchNode *>(index.internalPointer());
}

QModelIndex BranchModel::nodeToIndex(BranchNode *node) const
{
    if (node == m_rootNode)
        return QModelIndex();
    return createIndex(node->parent->rowOf(node), 0, static_cast<void *>(node));
}

void BranchModel::removeNode(const QModelIndex &idx)
{
    QModelIndex nodeIndex = idx; // idx is a leaf, so count must be 0.
    BranchNode *node = indexToNode(nodeIndex);
    while (node->count() == 0 && node->parent != m_rootNode) {
        BranchNode *parentNode = node->parent;
        const QModelIndex parentIndex = nodeToIndex(parentNode);
        const int nodeRow = nodeIndex.row();
        beginRemoveRows(parentIndex, nodeRow, nodeRow);
        parentNode->children.removeAt(nodeRow);
        delete node;
        endRemoveRows();
        node = parentNode;
        nodeIndex = parentIndex;
    }
}

QString BranchModel::toolTip(const QString &sha) const
{
    // Show the sha description excluding diff as toolTip
    QString output;
    QString errorMessage;
    QStringList arguments(QLatin1String("-n1"));
    arguments << sha;
    if (!m_client->synchronousLog(m_workingDirectory, arguments, &output, &errorMessage,
                                  VcsCommand::SuppressCommandLogging)) {
        return errorMessage;
    }
    return output;
}

} // namespace Internal
} // namespace Git

