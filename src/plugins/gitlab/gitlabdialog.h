// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#pragma once

#include "queryrunner.h"
#include "resultparser.h"

#include <utils/filepath.h>
#include <utils/id.h>

#include <QDialog>

QT_BEGIN_NAMESPACE
class QAbstractItemModel;
class QComboBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QToolButton;
class QTreeView;
QT_END_NAMESPACE

namespace GitLab {

class GitLabParameters;
class GitLabDialog : public QDialog
{
    Q_OBJECT
public:
    explicit GitLabDialog(QWidget *parent = nullptr);

    void updateRemotes();

protected:
    void keyPressEvent(QKeyEvent *event) override;

private:
    void resetTreeView(QTreeView *treeView, QAbstractItemModel *model);
    void requestMainViewUpdate();
    void updatePageButtons();

    void queryFirstPage();
    void queryPreviousPage();
    void queryNextPage();
    void queryLastPage();
    void querySearch();
    void continuePageUpdate();

    void handleUser(const User &user);
    void handleProjects(const Projects &projects);
    void fetchProjects();

    void cloneSelected();

    QPushButton *m_clonePB = nullptr;
    Utils::Id m_currentServerId;
    Query m_lastTreeViewQuery;
    PageInformation m_lastPageInformation;
    int m_currentUserId = -1;

    QLabel *m_mainLabel;
    QLabel *m_detailsLabel;
    QComboBox *m_remoteComboBox;
    QLabel *m_treeViewTitle;
    QLineEdit *m_searchLineEdit;
    QTreeView *m_treeView;
    QToolButton *m_firstToolButton;
    QToolButton *m_previousToolButton;
    QLabel *m_currentPageLabel;
    QToolButton *m_nextToolButton;
    QToolButton *m_lastToolButton;
};

} // namespace GitLab
