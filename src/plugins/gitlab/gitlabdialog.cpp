// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "gitlabdialog.h"

#include "gitlabclonedialog.h"
#include "gitlabparameters.h"
#include "gitlabplugin.h"
#include "gitlabprojectsettings.h"
#include "gitlabtr.h"

#include <projectexplorer/projectmanager.h>

#include <texteditor/fontsettings.h>
#include <texteditor/texteditorsettings.h>

#include <utils/layoutbuilder.h>
#include <utils/listmodel.h>
#include <utils/qtcassert.h>
#include <utils/utilsicons.h>

#include <QKeyEvent>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QRegularExpression>
#include <QSyntaxHighlighter>

#include <QAbstractButton>
#include <QApplication>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpacerItem>
#include <QToolButton>
#include <QTreeView>

using namespace Utils;

namespace GitLab {

GitLabDialog::GitLabDialog(QWidget *parent)
    : QDialog(parent)
    , m_lastTreeViewQuery(Query::NoQuery)
{
    setWindowTitle(Tr::tr("GitLab"));
    resize(665, 530);

    m_mainLabel = new QLabel;
    m_detailsLabel = new QLabel;

    m_remoteComboBox = new QComboBox(this);
    m_remoteComboBox->setMinimumSize(QSize(200, 0));

    m_treeViewTitle = new QLabel;

    m_searchLineEdit = new QLineEdit(this);
    m_searchLineEdit->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    m_searchLineEdit->setPlaceholderText(Tr::tr("Search"));

    auto searchPB = new QPushButton(Tr::tr("Search"));
    searchPB->setDefault(true);

    m_treeView = new QTreeView(this);
    m_treeView->setRootIsDecorated(false);
    m_treeView->setUniformRowHeights(true);
    m_treeView->setItemsExpandable(false);
    m_treeView->setExpandsOnDoubleClick(false);
    m_treeView->header()->setVisible(false);

    m_firstToolButton = new QToolButton(this);
    m_firstToolButton->setText(QString::fromUtf8("|<"));

    m_previousToolButton = new QToolButton(this);
    m_previousToolButton->setText(Tr::tr("..."));

    m_currentPageLabel = new QLabel(this);
    m_currentPageLabel->setText(Tr::tr("0"));

    m_nextToolButton = new QToolButton(this);
    m_nextToolButton->setText(Tr::tr("..."));

    m_lastToolButton = new QToolButton(this);
    m_lastToolButton->setText(QString::fromUtf8(">|"));

    m_clonePB = new QPushButton(Utils::Icons::DOWNLOAD.icon(), Tr::tr("Clone..."), this);
    m_clonePB->setEnabled(false);

    auto buttonBox = new QDialogButtonBox(this);
    buttonBox->setStandardButtons(QDialogButtonBox::Close);
    buttonBox->addButton(m_clonePB, QDialogButtonBox::ActionRole);

    using namespace Layouting;

    Column {
        Row {
            Column {
                m_mainLabel,
                m_detailsLabel
            },
            st,
            Tr::tr("Remote:"),
            m_remoteComboBox
        },
        Space(40),
        Row {
            m_treeViewTitle,
            st,
            m_searchLineEdit,
            searchPB
        },
        m_treeView,
        Row {
            st,
            m_firstToolButton,
            m_previousToolButton,
            m_currentPageLabel,
            m_nextToolButton,
            m_lastToolButton,
            st,
        },
        buttonBox
    }.attachTo(this);

    updateRemotes();

    connect(m_remoteComboBox, &QComboBox::currentIndexChanged,
            this, &GitLabDialog::requestMainViewUpdate);
    connect(m_searchLineEdit, &QLineEdit::returnPressed, this, &GitLabDialog::querySearch);
    connect(searchPB, &QPushButton::clicked, this, &GitLabDialog::querySearch);
    connect(m_clonePB, &QPushButton::clicked, this, &GitLabDialog::cloneSelected);
    connect(m_firstToolButton, &QToolButton::clicked, this, &GitLabDialog::queryFirstPage);
    connect(m_previousToolButton, &QToolButton::clicked, this, &GitLabDialog::queryPreviousPage);
    connect(m_nextToolButton, &QToolButton::clicked, this, &GitLabDialog::queryNextPage);
    connect(m_lastToolButton, &QToolButton::clicked, this, &GitLabDialog::queryLastPage);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

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
    m_remoteComboBox->clear();
    const GitLabParameters *global = GitLabPlugin::globalParameters();
    for (const GitLabServer &server : std::as_const(global->gitLabServers))
        m_remoteComboBox->addItem(server.displayString(), QVariant::fromValue(server));

    m_remoteComboBox->setCurrentIndex(m_remoteComboBox->findData(
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

    m_mainLabel->setText({});
    m_detailsLabel->setText({});
    m_treeViewTitle->setText({});
    m_searchLineEdit->setText({});
    resetTreeView(m_treeView, nullptr);
    updatePageButtons();

    bool linked = false;
    m_currentServerId = Id();
    if (auto project = ProjectExplorer::ProjectManager::startupProject()) {
        GitLabProjectSettings *projSettings = GitLabPlugin::projectSettings(project);
        if (projSettings->isLinked()) {
            m_currentServerId = projSettings->currentServer();
            linked = true;
        }
    }
    if (!m_currentServerId.isValid())
        m_currentServerId = m_remoteComboBox->currentData().value<GitLabServer>().id;
    if (m_currentServerId.isValid()) {
        const GitLabParameters *global = GitLabPlugin::globalParameters();
        const GitLabServer server = global->serverForId(m_currentServerId);
        m_remoteComboBox->setCurrentIndex(m_remoteComboBox->findData(QVariant::fromValue(server)));
    }
    m_remoteComboBox->setEnabled(!linked);

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
        m_currentPageLabel->setVisible(false);
        m_firstToolButton->setVisible(false);
        m_lastToolButton->setVisible(false);
        m_previousToolButton->setVisible(false);
        m_nextToolButton->setVisible(false);
    } else {
        m_currentPageLabel->setText(QString::number(m_lastPageInformation.currentPage));
        m_currentPageLabel->setVisible(true);
        m_firstToolButton->setVisible(true);
        m_lastToolButton->setVisible(true);
    }
    if (m_lastPageInformation.currentPage > 1) {
        m_firstToolButton->setEnabled(true);
        m_previousToolButton->setText(QString::number(m_lastPageInformation.currentPage - 1));
        m_previousToolButton->setVisible(true);
    } else {
        m_firstToolButton->setEnabled(false);
        m_previousToolButton->setVisible(false);
    }
    if (m_lastPageInformation.currentPage < m_lastPageInformation.totalPages) {
        m_lastToolButton->setEnabled(true);
        m_nextToolButton->setText(QString::number(m_lastPageInformation.currentPage + 1));
        m_nextToolButton->setVisible(true);
    } else {
        m_lastToolButton->setEnabled(false);
        m_nextToolButton->setVisible(false);
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
    m_lastTreeViewQuery.setAdditionalParameters({"search=" + m_searchLineEdit->text()});
    fetchProjects();
}

void GitLabDialog::handleUser(const User &user)
{
    m_lastPageInformation = {};
    m_currentUserId = user.id;

    if (!user.error.message.isEmpty()) {
        m_mainLabel->setText(Tr::tr("Not logged in."));
        if (user.error.code == 1) {
            m_detailsLabel->setText(Tr::tr("Insufficient access token."));
            m_detailsLabel->setToolTip(user.error.message + QLatin1Char('\n')
                                          + Tr::tr("Permission scope read_api or api needed."));
        } else if (user.error.code >= 300 && user.error.code < 400) {
            m_detailsLabel->setText(Tr::tr("Check settings for misconfiguration."));
            m_detailsLabel->setToolTip(user.error.message);
        } else {
            m_detailsLabel->setText({});
            m_detailsLabel->setToolTip({});
        }
        updatePageButtons();
        m_treeViewTitle->setText(Tr::tr("Projects (%1)").arg(0));
        return;
    }

    if (user.id != -1) {
        if (user.bot) {
            m_mainLabel->setText(Tr::tr("Using project access token."));
            m_detailsLabel->setText({});
        } else {
            m_mainLabel->setText(Tr::tr("Logged in as %1").arg(user.name));
            m_detailsLabel->setText(Tr::tr("Id: %1 (%2)").arg(user.id).arg(user.email));
        }
        m_detailsLabel->setToolTip({});
    } else {
        m_mainLabel->setText(Tr::tr("Not logged in."));
        m_detailsLabel->setText({});
        m_detailsLabel->setToolTip({});
    }
    m_lastTreeViewQuery = Query(Query::Projects);
    fetchProjects();
}

void GitLabDialog::handleProjects(const Projects &projects)
{
    auto listModel = new ListModel<Project *>(this);
    for (const Project &project : projects.projects)
        listModel->appendItem(new Project(project));

    // TODO use a real model / delegate..?
    listModel->setDataAccessor([](Project *data, int /*column*/, int role) -> QVariant {
        if (role == Qt::DisplayRole)
            return data->displayName;
        if (role == Qt::UserRole)
            return QVariant::fromValue(*data);
        return QVariant();
    });
    resetTreeView(m_treeView, listModel);
    int count = projects.error.message.isEmpty() ? projects.pageInfo.total : 0;
    m_treeViewTitle->setText(Tr::tr("Projects (%1)").arg(count));

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
    const QModelIndexList indexes = m_treeView->selectionModel()->selectedIndexes();
    QTC_ASSERT(indexes.size() == 1, return);
    const Project project = indexes.first().data(Qt::UserRole).value<Project>();
    QTC_ASSERT(!project.sshUrl.isEmpty() && !project.httpUrl.isEmpty(), return);
    GitLabCloneDialog dialog(project, this);
    if (dialog.exec() == QDialog::Accepted)
        reject();
}

} // namespace GitLab
