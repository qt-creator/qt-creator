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

#include "gitlabdialog.h"

#include "gitlabclonedialog.h"
#include "gitlabparameters.h"
#include "gitlabplugin.h"
#include "gitlabprojectsettings.h"

#include <projectexplorer/session.h>
#include <texteditor/fontsettings.h>
#include <texteditor/texteditorsettings.h>
#include <utils/listmodel.h>
#include <utils/qtcassert.h>
#include <utils/utilsicons.h>

#include <QKeyEvent>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QRegularExpression>
#include <QSyntaxHighlighter>

namespace GitLab {

GitLabDialog::GitLabDialog(QWidget *parent)
    : QDialog(parent)
    , m_lastTreeViewQuery(Query::NoQuery)
{
    m_ui.setupUi(this);
    m_clonePB = new QPushButton(Utils::Icons::DOWNLOAD.icon(), tr("Clone..."), this);
    m_ui.buttonBox->addButton(m_clonePB, QDialogButtonBox::ActionRole);
    m_clonePB->setEnabled(false);

    updateRemotes();

    connect(m_ui.remoteCB, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &GitLabDialog::requestMainViewUpdate);
    connect(m_ui.searchLE, &QLineEdit::returnPressed, this, &GitLabDialog::querySearch);
    connect(m_ui.searchPB, &QPushButton::clicked, this, &GitLabDialog::querySearch);
    connect(m_clonePB, &QPushButton::clicked, this, &GitLabDialog::cloneSelected);
    connect(m_ui.firstTB, &QToolButton::clicked, this, &GitLabDialog::queryFirstPage);
    connect(m_ui.previousTB, &QToolButton::clicked, this, &GitLabDialog::queryPreviousPage);
    connect(m_ui.nextTB, &QToolButton::clicked, this, &GitLabDialog::queryNextPage);
    connect(m_ui.lastTB, &QToolButton::clicked, this, &GitLabDialog::queryLastPage);
    requestMainViewUpdate();
}

void GitLabDialog::resetTreeView(QTreeView *treeView, QAbstractItemModel *model)
{
    auto oldModel = treeView->model();
    treeView->setModel(model);
    delete oldModel;
    if (model) {
        connect(treeView->selectionModel(), &QItemSelectionModel::selectionChanged,
                this, [this](const QItemSelection &selected, const QItemSelection &) {
            m_clonePB->setEnabled(!selected.isEmpty());
        });
        m_clonePB->setEnabled(!treeView->selectionModel()->selectedIndexes().isEmpty());
    }
}

void GitLabDialog::updateRemotes()
{
    m_ui.remoteCB->clear();
    const GitLabParameters *global = GitLabPlugin::globalParameters();
    for (const GitLabServer &server : qAsConst(global->gitLabServers))
        m_ui.remoteCB->addItem(server.displayString(), QVariant::fromValue(server));

    m_ui.remoteCB->setCurrentIndex(m_ui.remoteCB->findData(
                                       QVariant::fromValue(global->currentDefaultServer())));
}

void GitLabDialog::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return)
        return;
    QDialog::keyPressEvent(event);
}

void GitLabDialog::requestMainViewUpdate()
{
    m_lastPageInformation = PageInformation();
    m_lastTreeViewQuery = Query(Query::NoQuery);

    m_ui.mainLabel->setText({});
    m_ui.detailsLabel->setText({});
    m_ui.treeViewTitle->setText({});
    m_ui.searchLE->setText({});
    resetTreeView(m_ui.treeView, nullptr);
    updatePageButtons();

    bool linked = false;
    m_currentServerId = Utils::Id();
    if (auto project = ProjectExplorer::SessionManager::startupProject()) {
        GitLabProjectSettings *projSettings = GitLabPlugin::projectSettings(project);
        if (projSettings->isLinked()) {
            m_currentServerId = projSettings->currentServer();
            linked = true;
        }
    }
    if (!m_currentServerId.isValid())
        m_currentServerId = m_ui.remoteCB->currentData().value<GitLabServer>().id;
    if (m_currentServerId.isValid()) {
        const GitLabParameters *global = GitLabPlugin::globalParameters();
        const GitLabServer server = global->serverForId(m_currentServerId);
        m_ui.remoteCB->setCurrentIndex(m_ui.remoteCB->findData(QVariant::fromValue(server)));
    }
    m_ui.remoteCB->setEnabled(!linked);

    if (!m_currentServerId.isValid())
        return;

    const Query query(Query::User);
    QueryRunner *runner = new QueryRunner(query, m_currentServerId, this);
    connect(runner, &QueryRunner::resultRetrieved, this, [this](const QByteArray &result) {
        handleUser(ResultParser::parseUser(result));
    });
    connect(runner, &QueryRunner::finished, [runner]() { runner->deleteLater(); });
    runner->start();
}

void GitLabDialog::updatePageButtons()
{
    if (m_lastPageInformation.currentPage == -1) {
        m_ui.currentPage->setVisible(false);
        m_ui.firstTB->setVisible(false);
        m_ui.lastTB->setVisible(false);
        m_ui.previousTB->setVisible(false);
        m_ui.nextTB->setVisible(false);
    } else {
        m_ui.currentPage->setText(QString::number(m_lastPageInformation.currentPage));
        m_ui.currentPage->setVisible(true);
        m_ui.firstTB->setVisible(true);
        m_ui.lastTB->setVisible(true);
    }
    if (m_lastPageInformation.currentPage > 1) {
        m_ui.firstTB->setEnabled(true);
        m_ui.previousTB->setText(QString::number(m_lastPageInformation.currentPage - 1));
        m_ui.previousTB->setVisible(true);
    } else {
        m_ui.firstTB->setEnabled(false);
        m_ui.previousTB->setVisible(false);
    }
    if (m_lastPageInformation.currentPage < m_lastPageInformation.totalPages) {
        m_ui.lastTB->setEnabled(true);
        m_ui.nextTB->setText(QString::number(m_lastPageInformation.currentPage + 1));
        m_ui.nextTB->setVisible(true);
    } else {
        m_ui.lastTB->setEnabled(false);
        m_ui.nextTB->setVisible(false);
    }
}

void GitLabDialog::queryFirstPage()
{
    QTC_ASSERT(m_lastTreeViewQuery.type() != Query::NoQuery, return);
    QTC_ASSERT(m_lastPageInformation.currentPage != -1, return);
    m_lastTreeViewQuery.setPageParameter(1);
    fetchProjects();
}

void GitLabDialog::queryPreviousPage()
{
    QTC_ASSERT(m_lastTreeViewQuery.type() != Query::NoQuery, return);
    QTC_ASSERT(m_lastPageInformation.currentPage != -1, return);
    m_lastTreeViewQuery.setPageParameter(m_lastPageInformation.currentPage - 1);
    fetchProjects();
}

void GitLabDialog::queryNextPage()
{
    QTC_ASSERT(m_lastTreeViewQuery.type() != Query::NoQuery, return);
    QTC_ASSERT(m_lastPageInformation.currentPage != -1, return);
    m_lastTreeViewQuery.setPageParameter(m_lastPageInformation.currentPage + 1);
    fetchProjects();
}

void GitLabDialog::queryLastPage()
{
    QTC_ASSERT(m_lastTreeViewQuery.type() != Query::NoQuery, return);
    QTC_ASSERT(m_lastPageInformation.currentPage != -1, return);
    m_lastTreeViewQuery.setPageParameter(m_lastPageInformation.totalPages);
    fetchProjects();
}

void GitLabDialog::querySearch()
{
    QTC_ASSERT(m_lastTreeViewQuery.type() != Query::NoQuery, return);
    m_lastTreeViewQuery.setPageParameter(-1);
    m_lastTreeViewQuery.setAdditionalParameters({"search=" + m_ui.searchLE->text()});
    fetchProjects();
}

void GitLabDialog::handleUser(const User &user)
{
    m_lastPageInformation = {};
    m_currentUserId = user.id;

    if (!user.error.message.isEmpty()) {
        m_ui.mainLabel->setText(tr("Not logged in."));
        if (user.error.code == 1) {
            m_ui.detailsLabel->setText(tr("Insufficient access token."));
            m_ui.detailsLabel->setToolTip(user.error.message + QLatin1Char('\n')
                                          + tr("Permission scope read_api or api needed."));
        } else if (user.error.code >= 300 && user.error.code < 400) {
            m_ui.detailsLabel->setText(tr("Check settings for misconfiguration."));
            m_ui.detailsLabel->setToolTip(user.error.message);
        } else {
            m_ui.detailsLabel->setText({});
            m_ui.detailsLabel->setToolTip({});
        }
        updatePageButtons();
        m_ui.treeViewTitle->setText(tr("Projects (%1)").arg(0));
        return;
    }

    if (user.id != -1) {
        if (user.bot) {
            m_ui.mainLabel->setText(tr("Using project access token."));
            m_ui.detailsLabel->setText({});
        } else {
            m_ui.mainLabel->setText(tr("Logged in as %1").arg(user.name));
            m_ui.detailsLabel->setText(tr("Id: %1 (%2)").arg(user.id).arg(user.email));
        }
        m_ui.detailsLabel->setToolTip({});
    } else {
        m_ui.mainLabel->setText(tr("Not logged in."));
        m_ui.detailsLabel->setText({});
        m_ui.detailsLabel->setToolTip({});
    }
    m_lastTreeViewQuery = Query(Query::Projects);
    fetchProjects();
}

void GitLabDialog::handleProjects(const Projects &projects)
{
    Utils::ListModel<Project *> *listModel = new Utils::ListModel<Project *>(this);
    for (const Project &project : projects.projects)
        listModel->appendItem(new Project(project));

    // TODO use a real model / delegate..?
    listModel->setDataAccessor([](Project *data, int /*column*/, int role) -> QVariant {
        if (role == Qt::DisplayRole)
            return QString(data->displayName + " (" + data->visibility + ')');
        if (role == Qt::UserRole)
            return QVariant::fromValue(*data);
        return QVariant();
    });
    resetTreeView(m_ui.treeView, listModel);
    int count = projects.error.message.isEmpty() ? projects.pageInfo.total : 0;
    m_ui.treeViewTitle->setText(tr("Projects (%1)").arg(count));

    m_lastPageInformation = projects.pageInfo;
    updatePageButtons();
}

void GitLabDialog::fetchProjects()
{
    QueryRunner *runner = new QueryRunner(m_lastTreeViewQuery, m_currentServerId, this);
    connect(runner, &QueryRunner::resultRetrieved, this, [this](const QByteArray &result) {
        handleProjects(ResultParser::parseProjects(result));
    });
    connect(runner, &QueryRunner::finished, [runner]() { runner->deleteLater(); });
    runner->start();
}

void GitLabDialog::cloneSelected()
{
    const QModelIndexList indexes = m_ui.treeView->selectionModel()->selectedIndexes();
    QTC_ASSERT(indexes.size() == 1, return);
    const Project project = indexes.first().data(Qt::UserRole).value<Project>();
    QTC_ASSERT(!project.sshUrl.isEmpty() && !project.httpUrl.isEmpty(), return);
    GitLabCloneDialog dialog(project, this);
    if (dialog.exec() == QDialog::Accepted)
        reject();
}

} // namespace GitLab
