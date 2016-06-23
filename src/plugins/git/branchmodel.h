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

#pragma once

#include <QAbstractListModel>
#include <QVariant>

namespace Git {
namespace Internal {

class GitClient;

class BranchNode;

// --------------------------------------------------------------------------
// BranchModel:
// --------------------------------------------------------------------------

class BranchModel : public QAbstractItemModel {
    Q_OBJECT

public:
    explicit BranchModel(GitClient *client, QObject *parent = nullptr);
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
    bool refresh(const QString &workingDirectory, QString *errorMessage);

    void renameBranch(const QString &oldName, const QString &newName);
    void renameTag(const QString &oldName, const QString &newName);

    QString workingDirectory() const;
    GitClient *client() const;

    QModelIndex currentBranch() const;
    QString fullName(const QModelIndex &idx, bool includePrefix = false) const;
    QStringList localBranchNames() const;
    QString sha(const QModelIndex &idx) const;
    QDateTime dateTime(const QModelIndex &idx) const;
    bool hasTags() const;
    bool isLocal(const QModelIndex &idx) const;
    bool isLeaf(const QModelIndex &idx) const;
    bool isTag(const QModelIndex &idx) const;

    void removeBranch(const QModelIndex &idx);
    void removeTag(const QModelIndex &idx);
    void checkoutBranch(const QModelIndex &idx);
    bool branchIsMerged(const QModelIndex &idx);
    QModelIndex addBranch(const QString &name, bool track, const QModelIndex &trackedBranch);
    void setRemoteTracking(const QModelIndex &trackingIndex);
    void setOldBranchesIncluded(bool value);

private:
    void parseOutputLine(const QString &line);
    void setCurrentBranch();
    BranchNode *indexToNode(const QModelIndex &index) const;
    QModelIndex nodeToIndex(BranchNode *node, int column) const;
    void removeNode(const QModelIndex &idx);

    QString toolTip(const QString &sha) const;

    GitClient *m_client;
    QString m_workingDirectory;
    BranchNode *m_rootNode;
    BranchNode *m_currentBranch = nullptr;
    QString m_currentSha;
    QStringList m_obsoleteLocalBranches;
    bool m_oldBranchesIncluded = false;
};

} // namespace Internal
} // namespace Git
