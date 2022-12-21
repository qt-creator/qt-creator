// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/filepath.h>

#include <QDialog>

QT_BEGIN_NAMESPACE
class QLabel;
class QPushButton;
class QSortFilterProxyModel;
QT_END_NAMESPACE

namespace Utils { class TreeView; }

namespace Git::Internal {

class StashModel;

/* StashDialog: Non-modal dialog that manages the list of stashes
 * of the repository. Offers to show, restore, restore to branch
 * (in case restore fails due to conflicts) on current and
 * delete on selection/all. */

class StashDialog : public QDialog
{
public:
    explicit StashDialog(QWidget *parent = nullptr);
    ~StashDialog() override;

    void refresh(const Utils::FilePath &repository, bool force);

private:
    // Prompt dialog for modified repositories. Ask to undo or stash away.
    enum ModifiedRepositoryAction {
        ModifiedRepositoryCancel,
        ModifiedRepositoryStash,
        ModifiedRepositoryDiscard
    };

    void deleteAll();
    void deleteSelection();
    void showCurrent();
    void restoreCurrent();
    void restoreCurrentInBranch();
    void enableButtons();
    void forceRefresh();

    ModifiedRepositoryAction promptModifiedRepository(const QString &stash);
    bool promptForRestore(QString *stash, QString *branch /* = 0 */, QString *errorMessage);
    bool ask(const QString &title, const QString &what, bool defaultButton = true);
    void warning(const QString &title, const QString &what, const QString &details = QString());
    int currentRow() const;
    QList<int> selectedRows() const;    \

    StashModel *m_model;
    QSortFilterProxyModel *m_proxyModel;
    QPushButton *m_deleteAllButton;
    QPushButton *m_deleteSelectionButton;
    QPushButton *m_showCurrentButton;
    QPushButton *m_restoreCurrentButton;
    QPushButton *m_restoreCurrentInBranchButton;
    QPushButton *m_refreshButton;
    Utils::FilePath m_repository;

    QLabel *m_repositoryLabel;
    Utils::TreeView *m_stashView;
};

} // Git::Internal
