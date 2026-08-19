// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/filepath.h>

#include <QtTaskTree/QSingleTaskTreeRunner>

#include <QAbstractItemModel>
#include <QDateTime>
#include <QHash>

namespace Git::Internal {

class GraphRef
{
public:
    enum Type { Head, LocalBranch, RemoteBranch, Tag };

    QString name;
    Type type = LocalBranch;

    friend bool operator==(const GraphRef &, const GraphRef &) = default;
};

class GraphEdge
{
public:
    enum Type {
        Pass,      // passes through the row, bending over when its lane moved
        ToCommit,  // enters at the row top and ends at this row's node
        FromCommit // starts at this row's node and leaves at the row bottom
    };

    int fromLane = 0;
    int toLane = 0;
    int colorIndex = 0;
    Type type = Pass;

    friend bool operator==(const GraphEdge &, const GraphEdge &) = default;
};

class FileChange
{
public:
    QString status; // git status field, e.g. "M", "A", "D", "R100"
    QString path;
    QString oldPath; // for renames and copies
    int added = -1; // -1 for binary files, whose lines cannot be counted
    int deleted = -1;

    friend bool operator==(const FileChange &, const FileChange &) = default;
};

class CommitEntry
{
public:
    QString hash;
    QStringList parents;
    QList<GraphRef> refs;
    QString author;
    QDateTime authorDate;
    QString subject;
    QString message; // the complete commit message
    // Only filled by the per commit file query, GitGraphModel keeps the lists
    // of the commits whose row was expanded.
    QList<FileChange> files;
    // Filled by lane assignment:
    int lane = 0;
    int colorIndex = 0;
    QList<GraphEdge> edges;
    int laneCount = 1;

    friend bool operator==(const CommitEntry &, const CommitEntry &) = default;
};

// Two levels: commits at the top, each commit's changed files below it.
class GitGraphModel : public QAbstractItemModel
{
public:
    enum Role { HashRole = Qt::UserRole, FilePathRole, OldFilePathRole };

    explicit GitGraphModel(QObject *parent = nullptr);

    QModelIndex index(int row, int column, const QModelIndex &parent = {}) const override;
    QModelIndex parent(const QModelIndex &child) const override;
    int rowCount(const QModelIndex &parent = {}) const override;
    int columnCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    // The file lists are fetched per commit, when its row is expanded: they
    // cost an order of magnitude more than all commit headers together.
    bool hasChildren(const QModelIndex &parent = {}) const override;
    bool canFetchMore(const QModelIndex &parent) const override;
    void fetchMore(const QModelIndex &parent) override;

    void refresh(const Utils::FilePath &repository);
    Utils::FilePath repository() const { return m_repository; }

    void setAllBranches(bool allBranches) { m_allBranches = allBranches; }
    bool allBranches() const { return m_allBranches; }

    // Extends the history by a page. Does nothing while a page is on its way,
    // or when the log had nothing more to give.
    void loadMore();

    int commitCount() const { return int(m_entries.size()); }
    const CommitEntry &entryAt(int row) const { return m_entries.at(row); }
    const QList<FileChange> &filesAt(int commitRow) const;

private:
    void reload(bool nextPage = false);
    void loadSideBranches(const QList<CommitEntry> &entries, bool hasMore);
    void finishReload(QList<CommitEntry> entries, bool hasMore);
    void setEntries(const QList<CommitEntry> &entries, bool hasMore);
    void fetchNextFiles();
    void setFiles(const QString &hash, const QList<FileChange> &files);
    QList<CommitEntry> parseOutput(const QString &output) const;
    void sortTopologically(QList<CommitEntry> &entries) const;
    void assignLanes(QList<CommitEntry> &entries) const;

    QList<CommitEntry> m_entries;
    // The commits of the loaded window in the order git listed them, so that
    // "Load More Commits" can fetch the next page instead of the whole window.
    QList<CommitEntry> m_windowEntries;
    // Keyed by hash: what a commit changed cannot change, so the lists survive
    // a reload and switching between the repository's branches.
    QHash<QString, QList<FileChange>> m_filesByHash;
    QStringList m_pendingFiles;
    Utils::FilePath m_repository;
    QtTaskTree::QSingleTaskTreeRunner m_taskTreeRunner;
    QtTaskTree::QSingleTaskTreeRunner m_sideTaskTreeRunner;
    QtTaskTree::QSingleTaskTreeRunner m_filesTaskTreeRunner;
    // The current branch only, like gitk. Adding the other branches spreads
    // the graph over lanes that never join the history that is shown.
    bool m_allBranches = false;
    // Whether the log had more commits than the loaded window holds.
    bool m_hasMore = false;
    int m_maxCount = 0;
};

} // Git::Internal
