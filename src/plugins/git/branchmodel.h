// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/filepath.h>

#include <QAbstractListModel>

#include <optional>

namespace VcsBase {
class CommandResult;
}

namespace Git::Internal {

class BranchNode;

class BranchModel : public QAbstractItemModel
{
public:
    explicit BranchModel(QObject *parent = nullptr);
    ~BranchModel() override;

    // QAbstractItemModel
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &index) const override;
    int rowCount(const QModelIndex &parentIdx = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;

    void clear();
    enum class ShowError { No, Yes };
    void refresh(const Utils::FilePath &workingDirectory, ShowError showError = ShowError::No);

    void renameBranch(const QString &oldName, const QString &newName);
    void renameTag(const QString &oldName, const QString &newName);

    Utils::FilePath workingDirectory() const;

    QModelIndex currentBranch() const;
    QString fullName(const QModelIndex &idx, bool includePrefix = false) const;
    QStringList localBranchNames() const;
    QString sha(const QModelIndex &idx) const;
    QDateTime dateTime(const QModelIndex &idx) const;
    bool isHead(const QModelIndex &idx) const;
    bool isLocal(const QModelIndex &idx) const;
    bool isLeaf(const QModelIndex &idx) const;
    bool isTag(const QModelIndex &idx) const;

    void removeBranch(const QModelIndex &idx);
    void removeTag(const QModelIndex &idx);
    void checkoutBranch(const QModelIndex &idx, const QObject *context = nullptr,
                        const std::function<void(const VcsBase::CommandResult &)> &handler = {});
    bool branchIsMerged(const QModelIndex &idx);
    QModelIndex addBranch(const QString &name, bool track, const QModelIndex &trackedBranch);
    void setRemoteTracking(const QModelIndex &trackingIndex);
    void setOldBranchesIncluded(bool value);
    std::optional<QString> remoteName(const QModelIndex &idx) const;
    void refreshCurrentBranch();

private:
    void setCurrentBranch();
    BranchNode *indexToNode(const QModelIndex &index) const;
    QModelIndex nodeToIndex(BranchNode *node, int column) const;
    void removeNode(const QModelIndex &idx);
    void updateUpstreamStatus(BranchNode *node);

    QString toolTip(const QString &sha) const;

    class Private;
    Private *d;
};

} // Git::Internal
