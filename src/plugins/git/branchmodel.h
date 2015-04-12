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

#ifndef BRANCHMODEL_H
#define BRANCHMODEL_H

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
    explicit BranchModel(GitClient *client, QObject *parent = 0);
    ~BranchModel();

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

private:
    void parseOutputLine(const QString &line);
    void setCurrentBranch();
    BranchNode *indexToNode(const QModelIndex &index) const;
    QModelIndex nodeToIndex(BranchNode *node) const;
    void removeNode(const QModelIndex &idx);

    QString toolTip(const QString &sha) const;

    GitClient *m_client;
    QString m_workingDirectory;
    BranchNode *m_rootNode;
    BranchNode *m_currentBranch;
    QString m_currentSha;
};

} // namespace Internal
} // namespace Git

#endif // BRANCHMODEL_H
