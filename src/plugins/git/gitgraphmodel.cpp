// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "gitgraphmodel.h"

#include "gitclient.h"
#include "gitsettings.h"

#include <utils/algorithm.h>
#include <utils/processenums.h>
#include <utils/qtcassert.h>

#include <QLocale>
#include <QTextDocument>

using namespace Utils;
using namespace VcsBase;

namespace Git::Internal {

const int maxCommitCount = 1000;

static int pageSize()
{
    const int count = settings().logCount();
    return count <= 0 || count > maxCommitCount ? maxCommitCount : count;
}

// Commit records are delimited by \x1e, their fields by \x1f, so that the
// multi-line commit message stays parseable.
const char logFormat[] = "--format=%x1e%H%x1f%P%x1f%D%x1f%an%x1f%aI%x1f%B%x1e";

// --date-order picks the commits by their date, their order for the graph is
// established by sortTopologically().
static QStringList logArguments(int maxCount)
{
    return {"log", "--date-order", "-n", QString::number(maxCount), "--decorate=full", logFormat};
}

// --raw lists the files with their status, --numstat repeats them in the same
// order with the changed line counts. Both together cost more than the headers
// of the whole window, which is why they are asked for one commit at a time.
static QStringList fileArguments(const QString &hash)
{
    return {"log", "-n", "1", "--raw", "--numstat", logFormat, hash};
}

// Commit indexes carry internal id 0, file indexes their commit's row + 1.
GitGraphModel::GitGraphModel(QObject *parent)
    : QAbstractItemModel(parent)
{}

QModelIndex GitGraphModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent))
        return {};
    if (!parent.isValid())
        return createIndex(row, column, quintptr(0));
    return createIndex(row, column, quintptr(parent.row()) + 1);
}

QModelIndex GitGraphModel::parent(const QModelIndex &child) const
{
    if (!child.isValid() || child.internalId() == 0)
        return {};
    return createIndex(int(child.internalId()) - 1, 0, quintptr(0));
}

int GitGraphModel::rowCount(const QModelIndex &parent) const
{
    if (!parent.isValid())
        return int(m_entries.size());
    if (parent.internalId() != 0 || parent.row() < 0 || parent.row() >= m_entries.size())
        return 0;
    return int(filesAt(parent.row()).size());
}

int GitGraphModel::columnCount(const QModelIndex &) const
{
    return 1;
}

const QList<FileChange> &GitGraphModel::filesAt(int commitRow) const
{
    static const QList<FileChange> noFiles;
    const auto it = m_filesByHash.constFind(m_entries.at(commitRow).hash);
    return it == m_filesByHash.constEnd() ? noFiles : *it;
}

bool GitGraphModel::hasChildren(const QModelIndex &parent) const
{
    if (!parent.isValid())
        return rowCount() > 0;
    if (parent.internalId() != 0 || parent.row() < 0 || parent.row() >= m_entries.size())
        return false;
    // A commit whose files were not fetched yet is assumed to have some.
    return canFetchMore(parent) || !filesAt(parent.row()).isEmpty();
}

bool GitGraphModel::canFetchMore(const QModelIndex &parent) const
{
    // The commits are not offered here: QTreeView fetches during layout, no
    // matter where the view is scrolled to, and would read the whole history.
    // GitGraphView asks for the next page when the end is on screen.
    if (!parent.isValid() || parent.internalId() != 0)
        return false;
    if (parent.row() < 0 || parent.row() >= m_entries.size())
        return false;
    return !m_filesByHash.contains(m_entries.at(parent.row()).hash);
}

void GitGraphModel::fetchMore(const QModelIndex &parent)
{
    if (!canFetchMore(parent))
        return;
    const QString hash = m_entries.at(parent.row()).hash;
    // Newest first: walking the commits with the arrow keys expands each of them
    // in turn, and the one the user stopped at is the interesting one.
    if (!m_pendingFiles.contains(hash))
        m_pendingFiles.prepend(hash);
    fetchNextFiles();
}

void GitGraphModel::fetchNextFiles()
{
    if (m_filesTaskTreeRunner.isRunning() || m_pendingFiles.isEmpty() || m_repository.isEmpty())
        return;
    const QString hash = m_pendingFiles.takeFirst();
    const auto commandHandler = [this, hash](const CommandResult &result) {
        QList<FileChange> files;
        if (result.result() == ProcessResult::FinishedWithSuccess) {
            const QList<CommitEntry> entries = parseOutput(result.cleanedStdOut());
            if (!entries.isEmpty())
                files = entries.first().files;
        }
        setFiles(hash, files);
        fetchNextFiles();
    };
    m_filesTaskTreeRunner.start({gitClient().commandTask(
        {.workingDirectory = m_repository,
         .arguments = fileArguments(hash),
         .flags = RunFlag::NoOutput,
         .encoding = gitClient().encoding(GitClient::EncodingLogOutput, m_repository),
         .commandHandler = commandHandler})});
}

void GitGraphModel::setFiles(const QString &hash, const QList<FileChange> &files)
{
    if (m_filesByHash.contains(hash))
        return;
    const int row = Utils::indexOf(m_entries, [&hash](const CommitEntry &entry) {
        return entry.hash == hash;
    });
    if (row < 0) { // a reload dropped the commit while its files were fetched
        m_filesByHash.insert(hash, files);
        return;
    }
    const QModelIndex parent = index(row, 0);
    if (files.isEmpty()) {
        // Nothing to insert, but the row loses its expander. Merges land here,
        // git lists no files for them.
        emit layoutAboutToBeChanged({parent});
        m_filesByHash.insert(hash, files);
        emit layoutChanged({parent});
        return;
    }
    beginInsertRows(parent, 0, int(files.size()) - 1);
    m_filesByHash.insert(hash, files);
    endInsertRows();
}

QVariant GitGraphModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return {};
    if (index.internalId() != 0) {
        const int commitRow = int(index.internalId()) - 1;
        if (commitRow < 0 || commitRow >= m_entries.size())
            return {};
        const CommitEntry &entry = m_entries.at(commitRow);
        const QList<FileChange> &files = filesAt(commitRow);
        if (index.row() < 0 || index.row() >= files.size())
            return {};
        const FileChange &file = files.at(index.row());
        switch (role) {
        case Qt::DisplayRole:
            return file.path;
        case Qt::ToolTipRole: {
            QString tooltip = file.oldPath.isEmpty()
                                  ? QString("%1  %2").arg(file.status, file.path)
                                  : QString("%1  %2 -> %3").arg(file.status, file.oldPath,
                                                                file.path);
            if (file.added >= 0)
                tooltip += QString("  +%1/-%2").arg(file.added).arg(file.deleted);
            return Qt::convertFromPlainText(tooltip);
        }
        case HashRole:
            return entry.hash;
        case FilePathRole:
            return file.path;
        case OldFilePathRole:
            return file.oldPath;
        }
        return {};
    }

    const int row = index.row();
    if (row < 0 || row >= m_entries.size())
        return {};
    const CommitEntry &entry = m_entries.at(row);
    switch (role) {
    case Qt::DisplayRole:
        return entry.subject;
    case Qt::ToolTipRole:
        // The commit message is plain text, converting keeps QToolTip from
        // taking anything in it for markup.
        return Qt::convertFromPlainText(QString("%1  %2\n%3\n\n%4").arg(
            entry.hash.left(8), entry.author,
            QLocale().toString(entry.authorDate, QLocale::ShortFormat), entry.message));
    case HashRole:
        return entry.hash;
    }
    return {};
}

void GitGraphModel::refresh(const FilePath &repository)
{
    if (m_repository != repository || m_maxCount == 0)
        m_maxCount = pageSize();
    if (m_repository != repository) {
        m_repository = repository;
        m_filesByHash.clear();
        m_pendingFiles.clear();
        m_filesTaskTreeRunner.reset();
    }
    reload();
}

void GitGraphModel::loadMore()
{
    if (!m_hasMore || m_taskTreeRunner.isRunning() || m_sideTaskTreeRunner.isRunning())
        return;
    m_maxCount += pageSize();
    reload(true);
}

void GitGraphModel::reload(bool nextPage)
{
    if (m_taskTreeRunner.isRunning())
        m_taskTreeRunner.reset();
    if (m_sideTaskTreeRunner.isRunning())
        m_sideTaskTreeRunner.reset();
    if (m_repository.isEmpty()) {
        m_windowEntries.clear();
        setEntries({}, false);
        return;
    }

    // Only the commits that are not loaded yet, so that enlarging the window
    // does not read it from the start again.
    const int skip = nextPage ? int(m_windowEntries.size()) : 0;
    const int count = qMax(m_maxCount - skip, 1);
    // One commit beyond the limit tells whether more history is available.
    QStringList arguments = logArguments(count + 1);
    if (skip > 0)
        arguments << "--skip=" + QString::number(skip);
    if (m_allBranches) // "--all" would also bring in refs/stash and refs/notes
        arguments << "--branches" << "--remotes" << "--tags";

    const auto commandHandler = [this, nextPage, count](const CommandResult &result) {
        if (result.result() != ProcessResult::FinishedWithSuccess) {
            m_windowEntries.clear();
            setEntries({}, false);
            return;
        }
        QList<CommitEntry> entries = parseOutput(result.cleanedStdOut());
        const bool hasMore = entries.size() > count;
        if (hasMore)
            entries.resize(count);
        if (nextPage) {
            // Refs that moved since the previous page can shift the window,
            // and a shifted window repeats commits.
            QSet<QString> loaded;
            loaded.reserve(m_windowEntries.size());
            for (const CommitEntry &entry : std::as_const(m_windowEntries))
                loaded.insert(entry.hash);
            for (const CommitEntry &entry : std::as_const(entries)) {
                if (!loaded.contains(entry.hash))
                    m_windowEntries.append(entry);
            }
        } else {
            m_windowEntries = entries;
        }
        loadSideBranches(m_windowEntries, hasMore);
    };
    m_taskTreeRunner.start({gitClient().commandTask(
        {.workingDirectory = m_repository,
         .arguments = arguments,
         .flags = RunFlag::NoOutput,
         .encoding = gitClient().encoding(GitClient::EncodingLogOutput, m_repository),
         .commandHandler = commandHandler})});
}

// The window is limited by date, so the commits a merge brought in can lie
// outside of it and leave the merge without the branch it merged. Ask for
// exactly those, the set "git rev-list <merged> ^<mainline>" reports, and let
// sortTopologically() place them below their merge.
void GitGraphModel::loadSideBranches(const QList<CommitEntry> &entries, bool hasMore)
{
    const int maxMerges = 50;
    const int maxSideCommits = 500;

    QSet<QString> loaded;
    loaded.reserve(entries.size());
    for (const CommitEntry &entry : entries)
        loaded.insert(entry.hash);

    QStringList revisions;
    int merges = 0;
    for (const CommitEntry &entry : entries) {
        bool missing = false;
        for (int p = 1; p < entry.parents.size(); ++p) {
            if (!loaded.contains(entry.parents.at(p))) {
                revisions << entry.parents.at(p);
                missing = true;
            }
        }
        if (!missing)
            continue;
        revisions << ('^' + entry.parents.first());
        if (++merges == maxMerges)
            break;
    }
    if (revisions.isEmpty()) {
        finishReload(entries, hasMore);
        return;
    }

    const auto commandHandler = [this, entries, hasMore](const CommandResult &result) {
        QList<CommitEntry> all = entries;
        if (result.result() == ProcessResult::FinishedWithSuccess) {
            QSet<QString> known;
            known.reserve(all.size());
            for (const CommitEntry &entry : std::as_const(all))
                known.insert(entry.hash);
            // Appending is enough, sortTopologically() moves them into place.
            const QList<CommitEntry> sideEntries = parseOutput(result.cleanedStdOut());
            for (const CommitEntry &entry : sideEntries) {
                if (!known.contains(entry.hash))
                    all.append(entry);
            }
        }
        finishReload(all, hasMore);
    };
    m_sideTaskTreeRunner.start({gitClient().commandTask(
        {.workingDirectory = m_repository,
         .arguments = logArguments(maxSideCommits) + revisions,
         .flags = RunFlag::NoOutput,
         .encoding = gitClient().encoding(GitClient::EncodingLogOutput, m_repository),
         .commandHandler = commandHandler})});
}

void GitGraphModel::finishReload(QList<CommitEntry> entries, bool hasMore)
{
    sortTopologically(entries);
    assignLanes(entries);
    setEntries(entries, hasMore);
}

// Replaces the rows from the first commit that differs on, so that the leading
// commits keep their view state (expanded, selected, scroll position). Enlarging
// the window rewrites more than its tail whenever it now reaches the side branch
// of a merge, because sortTopologically() then moves those commits up under
// their merge.
void GitGraphModel::setEntries(const QList<CommitEntry> &entries, bool hasMore)
{
    m_hasMore = hasMore;
    if (entries == m_entries)
        return;

    const auto mismatch = std::mismatch(m_entries.cbegin(), m_entries.cend(),
                                        entries.cbegin(), entries.cend());
    const int common = int(mismatch.first - m_entries.cbegin());

    if (common < m_entries.size()) {
        beginRemoveRows({}, common, int(m_entries.size()) - 1);
        m_entries.resize(common);
        endRemoveRows();
    }
    if (common < entries.size()) {
        beginInsertRows({}, common, int(entries.size()) - 1);
        m_entries = entries;
        endInsertRows();
    }
}

// The file block holds the --raw lines, ":<modes> <hashes> <status>" followed by
// the paths, and then the --numstat lines, "<added>\t<deleted>\t<path>", for the
// same files in the same order.
static void parseFileBlock(const QString &block, CommitEntry &entry)
{
    const QStringList lines = block.split('\n', Qt::SkipEmptyParts);
    int numstatIndex = 0;
    for (const QString &line : lines) {
        const QStringList parts = line.split('\t');
        if (parts.size() < 2)
            continue;
        if (line.startsWith(':')) {
            FileChange change;
            change.status = parts.first().section(' ', -1);
            change.path = parts.last();
            if (parts.size() > 2)
                change.oldPath = parts.at(1);
            entry.files.append(change);
        } else if (numstatIndex < entry.files.size()) {
            FileChange &change = entry.files[numstatIndex++];
            bool addedOk = false;
            bool deletedOk = false;
            const int added = parts.at(0).toInt(&addedOk);
            const int deleted = parts.at(1).toInt(&deletedOk);
            if (addedOk && deletedOk) {
                change.added = added;
                change.deleted = deleted;
            }
        }
    }
}

QList<CommitEntry> GitGraphModel::parseOutput(const QString &output) const
{
    const QString headsPrefix = "refs/heads/";
    const QString remotesPrefix = "refs/remotes/";
    const QString tagPrefix = "tag: refs/tags/";
    const QString headPrefix = "HEAD -> ";

    // Commit records are delimited by \x1e, their fields by \x1f, so that the
    // multi-line commit message stays parseable. Splitting at the record
    // separator yields the leading remainder, then a commit header and its file
    // block in turn. A header carries all six fields; anything else is the file
    // block of the commit before it, which also resynchronizes the parsing on a
    // separator that appeared inside a commit message.
    const QStringList records = output.split('\x1e');
    QList<CommitEntry> entries;
    entries.reserve(records.size() / 2);
    for (int i = 1; i < records.size(); ++i) {
        const QStringList fields = records.at(i).split('\x1f');
        if (fields.size() < 6) {
            if (!entries.isEmpty())
                parseFileBlock(records.at(i), entries.last());
            continue;
        }
        CommitEntry entry;
        entry.hash = fields.at(0);
        entry.parents = fields.at(1).split(' ', Qt::SkipEmptyParts);
        entry.author = fields.at(3);
        entry.authorDate = QDateTime::fromString(fields.at(4), Qt::ISODate);
        // The message comes last, so a field separator in it does no harm.
        entry.message = fields.mid(5).join('\x1f').trimmed();
        const int subjectEnd = entry.message.indexOf('\n');
        entry.subject = subjectEnd < 0 ? entry.message : entry.message.left(subjectEnd);
        const QStringList decorations = fields.at(2).split(", ", Qt::SkipEmptyParts);
        for (const QString &decoration : decorations) {
            if (decoration == "HEAD") {
                entry.refs.append({decoration, GraphRef::Head});
            } else if (decoration.startsWith(headPrefix)) {
                QString name = decoration.mid(headPrefix.size());
                if (name.startsWith(headsPrefix))
                    name = name.mid(headsPrefix.size());
                entry.refs.append({name, GraphRef::Head});
            } else if (decoration.startsWith(tagPrefix)) {
                entry.refs.append({decoration.mid(tagPrefix.size()), GraphRef::Tag});
            } else if (decoration.startsWith(headsPrefix)) {
                entry.refs.append({decoration.mid(headsPrefix.size()), GraphRef::LocalBranch});
            } else if (decoration.startsWith(remotesPrefix)) {
                entry.refs.append({decoration.mid(remotesPrefix.size()), GraphRef::RemoteBranch});
            }
        }
        entries.append(entry);
    }
    return entries;
}

// Brings the commits into the order the graph is drawn in: every commit follows
// all of its children, and the commits of a merged in branch directly follow
// their merge instead of appearing at their own date much further down. This is
// the compact arrangement gitk shows, and it lets the lanes always run
// downwards, which assignLanes() relies on.
void GitGraphModel::sortTopologically(QList<CommitEntry> &entries) const
{
    QHash<QString, int> indexOfHash;
    indexOfHash.reserve(entries.size());
    for (int i = 0; i < entries.size(); ++i)
        indexOfHash.insert(entries.at(i).hash, i);

    // Children outside the window do not count, so that the commits along its
    // border stay eligible instead of waiting for something never loaded.
    QList<int> pendingChildren(entries.size(), 0);
    for (const CommitEntry &entry : std::as_const(entries)) {
        for (const QString &parent : entry.parents) {
            const auto it = indexOfHash.constFind(parent);
            if (it != indexOfHash.constEnd())
                ++pendingChildren[*it];
        }
    }

    QList<CommitEntry> sorted;
    sorted.reserve(entries.size());
    QList<bool> emitted(entries.size(), false);
    QList<int> stack;
    for (int i = 0; i < entries.size(); ++i) {
        if (emitted.at(i) || pendingChildren.at(i) > 0)
            continue;
        stack.append(i);
        while (!stack.isEmpty()) {
            const int current = stack.takeLast();
            // A commit still awaiting a child is dropped here. The child that
            // is emitted last brings it back onto the stack.
            if (emitted.at(current) || pendingChildren.at(current) > 0)
                continue;
            emitted[current] = true;
            const CommitEntry &entry = entries.at(current);
            sorted.append(entry);
            // In order, so that the additional parents of a merge, the branch
            // it merged in, are taken from the stack before the first parent
            // carries on with the mainline.
            for (const QString &parent : entry.parents) {
                const auto it = indexOfHash.constFind(parent);
                if (it == indexOfHash.constEnd())
                    continue;
                --pendingChildren[*it];
                stack.append(*it);
            }
        }
    }

    QTC_CHECK(sorted.size() == entries.size());
    for (int i = 0; i < entries.size(); ++i) { // no commit may get lost
        if (!emitted.at(i))
            sorted.append(entries.at(i));
    }
    entries = sorted;
}

void GitGraphModel::assignLanes(QList<CommitEntry> &entries) const
{
    class LaneSlot
    {
    public:
        QString awaitedHash; // the lane is free while empty
        int colorIndex = 0;
    };
    QList<LaneSlot> active;
    int nextColor = 0;

    const auto takeFreeLane = [&active](const QString &hash, int colorIndex) {
        for (int i = 0; i < active.size(); ++i) {
            if (active.at(i).awaitedHash.isEmpty()) {
                active[i] = {hash, colorIndex};
                return i;
            }
        }
        active.append({hash, colorIndex});
        return int(active.size()) - 1;
    };

    // Where the lanes were before they were packed, so that the edges of the
    // current row start where the row above them ended.
    QList<int> laneAbove;
    const auto above = [&laneAbove](int lane) {
        return lane < laneAbove.size() ? laneAbove.at(lane) : lane; // else new here
    };

    for (CommitEntry &entry : entries) {
        // The lanes stay packed to the left: what a closed lane left behind is
        // taken over by the lanes right of it, whose lines bend over inside
        // this row.
        laneAbove.clear();
        QList<LaneSlot> packed;
        for (int i = 0; i < active.size(); ++i) {
            if (active.at(i).awaitedHash.isEmpty())
                continue;
            packed.append(active.at(i));
            laneAbove.append(i);
        }
        active = packed;

        QList<int> myLanes;
        for (int i = 0; i < active.size(); ++i) {
            if (active.at(i).awaitedHash == entry.hash)
                myLanes.append(i);
        }
        const bool isTip = myLanes.isEmpty();
        if (isTip)
            myLanes.append(takeFreeLane(entry.hash, nextColor++));
        entry.lane = myLanes.first();
        entry.colorIndex = active.at(entry.lane).colorIndex;

        QSet<int> noPass; // lanes that end at, or start from, this row's node
        for (const int i : std::as_const(myLanes)) {
            if (!isTip) {
                entry.edges.append(
                    {above(i), entry.lane, active.at(i).colorIndex, GraphEdge::ToCommit});
            }
            noPass.insert(i);
            if (i != entry.lane)
                active[i].awaitedHash.clear();
        }

        if (entry.parents.isEmpty()) {
            active[entry.lane].awaitedHash.clear();
        } else {
            // The additional parents come first: as long as this row's lane
            // still awaits the commit itself, it cannot be handed out as a
            // free lane for one of them.
            for (int p = 1; p < entry.parents.size(); ++p) {
                const QString &parent = entry.parents.at(p);
                int lane = -1;
                for (int i = 0; i < active.size(); ++i) {
                    if (active.at(i).awaitedHash == parent) {
                        lane = i;
                        break;
                    }
                }
                if (lane < 0) {
                    lane = takeFreeLane(parent, nextColor++);
                    noPass.insert(lane); // the new lane starts at this node
                }
                entry.edges.append(
                    {entry.lane, lane, active.at(lane).colorIndex, GraphEdge::FromCommit});
            }
            // A parent the log did not reach keeps its lane to the last row:
            // the history continues outside of what was loaded.
            active[entry.lane].awaitedHash = entry.parents.first();
            entry.edges.append({entry.lane, entry.lane, entry.colorIndex, GraphEdge::FromCommit});
        }

        for (int i = 0; i < active.size(); ++i) {
            if (!active.at(i).awaitedHash.isEmpty() && !noPass.contains(i))
                entry.edges.append({above(i), i, active.at(i).colorIndex, GraphEdge::Pass});
        }

        int laneCount = entry.lane + 1;
        for (const GraphEdge &edge : std::as_const(entry.edges))
            laneCount = qMax(laneCount, qMax(edge.fromLane, edge.toLane) + 1);
        entry.laneCount = laneCount;
    }
}

} // Git::Internal
