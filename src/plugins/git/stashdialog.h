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

#include <QDialog>

QT_BEGIN_NAMESPACE
class QSortFilterProxyModel;
class QPushButton;
QT_END_NAMESPACE

namespace Git {
namespace Internal {

namespace Ui { class StashDialog; }
class StashModel;

/* StashDialog: Non-modal dialog that manages the list of stashes
 * of the repository. Offers to show, restore, restore to branch
 * (in case restore fails due to conflicts) on current and
 * delete on selection/all. */

class StashDialog : public QDialog
{
    Q_OBJECT

public:
    explicit StashDialog(QWidget *parent = 0);
    ~StashDialog() override;

    void refresh(const QString &repository, bool force);

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

    Ui::StashDialog *ui;
    StashModel *m_model;
    QSortFilterProxyModel *m_proxyModel;
    QPushButton *m_deleteAllButton;
    QPushButton *m_deleteSelectionButton;
    QPushButton *m_showCurrentButton;
    QPushButton *m_restoreCurrentButton;
    QPushButton *m_restoreCurrentInBranchButton;
    QPushButton *m_refreshButton;
    QString m_repository;
};

} // namespace Internal
} // namespace Git
