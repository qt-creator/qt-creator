/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "branchmodel.h"
#include "gitclient.h"

#include <utils/qtcassert.h>
#include <vcsbase/vcsbaseoutputwindow.h>

#include <QFont>

namespace Git {
namespace Internal {

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

    bool isTag() const
    {
        if (!parent)
            return false;
        for (const BranchNode *p = this; p->parent; p = p->parent) {
            // find root child with name "tags"
            if (!p->parent->parent && p->name == QLatin1String("tags"))
                return true;
        }
        return false;
    }

    bool childOf(BranchNode *node) const
    {
        if (this == node)
            return true;
        return parent ? parent->childOf(node) : false;
    }

    bool isLocal() const
    {
        BranchNode *rn = rootNode();
        if (rn->isLeaf())
            return false;
        return childOf(rn->children.at(0));
    }

    BranchNode *childOfName(const QString &name) const
    {
        for (int i = 0; i < children.count(); ++i) {
            if (children.at(i)->name == name)
                return children.at(i);
        }
        return 0;
    }

    QStringList fullName() const
    {
        QTC_ASSERT(isLeaf(), return QStringList());

        QStringList fn;
        QList<const BranchNode *> nodes;
        const BranchNode *current = this;
        while (current->parent) {
            nodes.prepend(current);
            current = current->parent;
        }

        if (current->children.at(0) == nodes.at(0))
            nodes.removeFirst(); // remove local branch designation

        foreach (const BranchNode *n, nodes)
            fn.append(n->name);

        return fn;
    }

    void insert(const QStringList path, BranchNode *n)
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
        return QStringList(fullName().join(QString(QLatin1Char('/'))));
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
    m_rootNode->append(new BranchNode(tr("Local Branches")));
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
        VcsBase::VcsBaseOutputWindow::instance()->appendError(errorMessage);
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
    while (m_rootNode->count() > 1)
        delete m_rootNode->children.takeLast();
    BranchNode *locals = m_rootNode->children.at(0);
    while (locals->count())
        delete locals->children.takeLast();

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
        VcsBase::VcsBaseOutputWindow::instance()->appendError(*errorMessage);

    m_workingDirectory = workingDirectory;
    const QStringList lines = output.split(QLatin1Char('\n'));
    foreach (const QString &l, lines)
        parseOutputLine(l);

    if (m_currentBranch) {
        if (m_currentBranch->parent == m_rootNode->children[0])
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

    BranchNode *local = m_rootNode->children.at(0);
    int pos = 0;
    for (pos = 0; pos < local->count(); ++pos) {
        if (local->children.at(pos)->name == currentBranch) {
            m_currentBranch = local->children[pos];
        }
    }
}

void BranchModel::renameBranch(const QString &oldName, const QString &newName)
{
    QString errorMessage;
    QString output;
    if (!m_client->synchronousBranchCmd(m_workingDirectory,
                                        QStringList() << QLatin1String("-m") << oldName << newName,
                                        &output, &errorMessage))
        VcsBase::VcsBaseOutputWindow::instance()->appendError(errorMessage);
    else
        refresh(m_workingDirectory, &errorMessage);
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

QString BranchModel::branchName(const QModelIndex &idx) const
{
    if (!idx.isValid())
        return QString();
    BranchNode *node = indexToNode(idx);
    if (!node || !node->isLeaf())
        return QString();
    QStringList path = node->fullName();
    return path.join(QString(QLatin1Char('/')));
}

QStringList BranchModel::localBranchNames() const
{
    if (!m_rootNode || m_rootNode->children.isEmpty())
        return QStringList();

    return m_rootNode->children.at(0)->childrenNames();
}

QString BranchModel::sha(const QModelIndex &idx) const
{
    if (!idx.isValid())
        return QString();
    BranchNode *node = indexToNode(idx);
    return node->sha;
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
    if (!idx.isValid())
        return false;
    return indexToNode(idx)->isTag();
}

void BranchModel::removeBranch(const QModelIndex &idx)
{
    QString branch = branchName(idx);
    if (branch.isEmpty())
        return;

    QString errorMessage;
    QString output;
    QStringList args;

    args << QLatin1String("-D") << branch;
    if (!m_client->synchronousBranchCmd(m_workingDirectory, args, &output, &errorMessage)) {
        VcsBase::VcsBaseOutputWindow::instance()->appendError(errorMessage);
        return;
    }

    QModelIndex tmp = idx; // tmp is a leaf, so count must be 0.
    while (indexToNode(tmp)->count() == 0) {
        QModelIndex tmpParent = parent(tmp);
        beginRemoveRows(tmpParent, tmp.row(), tmp.row());
        indexToNode(tmpParent)->children.removeAt(tmp.row());
        delete indexToNode(tmp);
        endRemoveRows();
        tmp = tmpParent;
    }
}

void BranchModel::checkoutBranch(const QModelIndex &idx)
{
    QString branch = branchName(idx);
    if (branch.isEmpty())
        return;

    // No StashGuard since this function for now is only used with clean working dir.
    // If it is ever used from another place, please add StashGuard here
    m_client->synchronousCheckout(m_workingDirectory, branch);
}

bool BranchModel::branchIsMerged(const QModelIndex &idx)
{
    QString branch = branchName(idx);
    if (branch.isEmpty())
        return false;

    QString errorMessage;
    QString output;
    QStringList args;

    args << QLatin1String("-a") << QLatin1String("--contains") << sha(idx);
    if (!m_client->synchronousBranchCmd(m_workingDirectory, args, &output, &errorMessage))
        VcsBase::VcsBaseOutputWindow::instance()->appendError(errorMessage);

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

    const QString trackedBranch = branchName(startPoint);
    QString output;
    QString errorMessage;

    QStringList args;
    args << (track ? QLatin1String("--track") : QLatin1String("--no-track"));
    args << name;
    if (!trackedBranch.isEmpty())
        args << trackedBranch;

    if (!m_client->synchronousBranchCmd(m_workingDirectory, args, &output, &errorMessage)) {
        VcsBase::VcsBaseOutputWindow::instance()->appendError(errorMessage);
        return QModelIndex();
    }

    BranchNode *local = m_rootNode->children.at(0);
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
    BranchNode *newNode = new BranchNode(leafName, sha(startPoint), track ? trackedBranch : QString());
    if (!added)
        beginInsertRows(nodeToIndex(local), pos, pos);
    newNode->parent = local;
    local->children.insert(pos, newNode);
    endInsertRows();
    return nodeToIndex(newNode);
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
    bool showTags = m_client->settings()->boolValue(GitSettings::showTagsKey);

    // insert node into tree:
    QStringList nameParts = fullName.split(QLatin1Char('/'));
    nameParts.removeFirst(); // remove refs...

    if (nameParts.first() == QLatin1String("heads"))
        nameParts[0] = m_rootNode->children.at(0)->name; // Insert the local designator
    else if (nameParts.first() == QLatin1String("remotes"))
        nameParts.removeFirst(); // remove "remotes"
    else if (nameParts.first() == QLatin1String("stash"))
        return;
    else if (!showTags && (nameParts.first() == QLatin1String("tags")))
        return;

    // limit depth of list. Git basically only ever wants one / and considers the rest as part of
    // the name.
    while (nameParts.count() > 3) {
        nameParts[2] = nameParts.at(2) + QLatin1Char('/') + nameParts.at(3);
        nameParts.removeAt(3);
    }

    const QString name = nameParts.last();
    nameParts.removeLast();

    BranchNode *newNode = new BranchNode(name, sha, lineParts.at(2));
    m_rootNode->insert(nameParts, newNode);
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

QString BranchModel::toolTip(const QString &sha) const
{
    // Show the sha description excluding diff as toolTip
    QString output;
    QString errorMessage;
    QStringList arguments(QLatin1String("-n1"));
    arguments << sha;
    if (!m_client->synchronousLog(m_workingDirectory, arguments, &output, &errorMessage))
        return errorMessage;
    return output;
}

} // namespace Internal
} // namespace Git

