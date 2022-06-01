/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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

#include "ui_gitlabdialog.h"

#include "queryrunner.h"
#include "resultparser.h"

#include <utils/filepath.h>
#include <utils/id.h>

#include <QDialog>

QT_BEGIN_NAMESPACE
class QPushButton;
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

    Ui::GitLabDialog m_ui;
    QPushButton *m_clonePB = nullptr;
    Utils::Id m_currentServerId;
    Query m_lastTreeViewQuery;
    PageInformation m_lastPageInformation;
    int m_currentUserId = -1;
};

} // namespace GitLab
