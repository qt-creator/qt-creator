/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "gitoriousprojectwidget.h"
#include "gitorioushostwizardpage.h"
#include "gitorious.h"
#include "ui_gitoriousprojectwidget.h"

#include <utils/qtcassert.h>

#include <QRegExp>
#include <QDebug>

#include <QStandardItemModel>
#include <QSortFilterProxyModel>
#include <QStandardItem>
#include <QItemSelectionModel>
#include <QDesktopServices>
#include <QIcon>
#include <QStyle>

enum {
    urlRole = Qt::UserRole + 1  // Project has a URL in the description
};

enum { debug = 1 };

namespace Gitorious {
namespace Internal {

enum { ProjectColumn, DescriptionColumn, ColumnCount };

GitoriousProjectWidget::GitoriousProjectWidget(int hostIndex,
                                               QWidget *parent) :
    QWidget(parent),
    m_hostName(Gitorious::instance().hostName(hostIndex)),
    ui(new Ui::GitoriousProjectWidget),
    m_model(new QStandardItemModel(0, ColumnCount, this)),
    m_filterModel(new QSortFilterProxyModel),
    m_valid(false)
{
    ui->setupUi(this);
    ui->infoToolButton->setIcon(style()->standardIcon(QStyle::SP_MessageBoxInformation));
    ui->infoToolButton->setEnabled(false);
    connect(ui->infoToolButton, SIGNAL(clicked()), this, SLOT(slotInfo()));
    // Filter
    connect(ui->filterLineEdit, SIGNAL(filterChanged(QString)), m_filterModel, SLOT(setFilterFixedString(QString)));
    // Updater
    ui->updateCheckBox->setChecked(true);
    if (Gitorious::instance().hostState(hostIndex) != GitoriousHost::ProjectsQueryRunning)
        ui->updateCheckBox->setVisible(false);
    connect(ui->updateCheckBox, SIGNAL(stateChanged(int)), this, SLOT(slotUpdateCheckBoxChanged(int)));
    // Model
    QStringList headers;
    headers << tr("Project") << tr("Description");
    m_model->setHorizontalHeaderLabels(headers);
    // Populate the model
    slotUpdateProjects(hostIndex);
    // Filter on all columns
    m_filterModel->setSourceModel(m_model);
    m_filterModel->setFilterKeyColumn(-1);
    m_filterModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    m_filterModel->setSortCaseSensitivity(Qt::CaseInsensitive);
    ui->projectTreeView->setModel(m_filterModel);
    // View
    ui->projectTreeView->setAlternatingRowColors(true);
    ui->projectTreeView->setRootIsDecorated(false);
    ui->projectTreeView->setUniformRowHeights(true);
    ui->projectTreeView->setSortingEnabled(true);
    connect(ui->projectTreeView->selectionModel(), SIGNAL(currentChanged(QModelIndex,QModelIndex)),
            this, SLOT(slotCurrentChanged(QModelIndex,QModelIndex)));
    ui->projectTreeView->setSelectionMode(QAbstractItemView::SingleSelection);
    // Select first, resize columns
    if (Gitorious::instance().projectCount(hostIndex)) {
        for (int r = 0; r < ColumnCount; r++)
            ui->projectTreeView->resizeColumnToContents(r);
        // Select first
        const QModelIndex index = m_filterModel->index(0, 0);
        ui->projectTreeView->selectionModel()->setCurrentIndex(index, QItemSelectionModel::Select|QItemSelectionModel::Current|QItemSelectionModel::Rows);
    }

    // Continuous update
    Gitorious *gitorious = &Gitorious::instance();
    connect(gitorious, SIGNAL(projectListPageReceived(int,int)), this, SLOT(slotUpdateProjects(int)));
    connect(gitorious, SIGNAL(projectListReceived(int)), this, SLOT(slotUpdateProjects(int)));
}

GitoriousProjectWidget::~GitoriousProjectWidget()
{
    Gitorious *gitorious = &Gitorious::instance();
    disconnect(gitorious, SIGNAL(projectListPageReceived(int,int)), this, SLOT(slotUpdateProjects(int)));
    disconnect(gitorious, SIGNAL(projectListReceived(int)), this, SLOT(slotUpdateProjects(int)));
    delete ui;
}

// Map indexes back via filter
QStandardItem *GitoriousProjectWidget::itemFromIndex(const QModelIndex &index) const
{
    if (index.isValid())
        return m_model->itemFromIndex(m_filterModel->mapToSource(index));
    return 0;
}

QStandardItem *GitoriousProjectWidget::currentItem() const
{
    return itemFromIndex(ui->projectTreeView->selectionModel()->currentIndex());
}

void GitoriousProjectWidget::slotCurrentChanged(const QModelIndex &current, const QModelIndex & /* previous */)
{
    // Any info URL to show?
    QString url;
    if (current.isValid())
        if (QStandardItem *item = itemFromIndex(current)) {
        // Project: URL in description?
        const QVariant urlV = item->data(urlRole);
        if (urlV.isValid())
            url = urlV.toString();
    }

    ui->infoToolButton->setEnabled(!url.isEmpty());
    ui->infoToolButton->setToolTip(url);

    const bool isValid = current.isValid();
    if (isValid != m_valid) {
        m_valid = isValid;
        emit validChanged();
    }
}

void GitoriousProjectWidget::slotInfo()
{
    if (const QStandardItem *item = currentItem()) {
        const QVariant url = item->data(urlRole);
        if (url.isValid())
            QDesktopServices::openUrl(QUrl(url.toString()));
    }
}

// Create a model row for a project
static inline QList<QStandardItem *> projectEntry(const GitoriousProject &p)
{
    enum { maxNameLength = 30 };
    // Truncate names with colons
    QString name = p.name;
    const int colonPos = name.indexOf(QLatin1Char(':'));
    if (colonPos != -1)
        name.truncate(colonPos);
    if (name.size() > maxNameLength) {
        name.truncate(maxNameLength);
        name += QLatin1String("...");
    }
    QStandardItem *nameItem = new QStandardItem(name);
    nameItem->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
    // Description
    QStandardItem *descriptionItem = new QStandardItem;
    descriptionItem->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
    QList<QStandardItem *> rc;
    rc << nameItem << descriptionItem;
    // Should the text contain an URL, store it under 'urlRole' for the info button
    QString url;
    GitoriousProjectWidget::setDescription(p.description, DescriptionColumn, &rc, &url);
    if (!url.isEmpty()) {
        const QVariant urlV = QVariant(url);
        nameItem->setData(urlV, urlRole);
        descriptionItem->setData(urlV, urlRole);
    }
    return rc;
}

// Utility to set description column and tooltip for a row from a free
// format/HTMLish gitorious description. Make sure the description is just one
// row for the item and set a tooltip with full contents. If desired, extract
// an URL.

void GitoriousProjectWidget::setDescription(const QString &description,
                                                int descriptionColumn,
                                                QList<QStandardItem *> *items,
                                                QString *url /* =0 */)
{
    enum { MaxDescriptionLineLength = 70 };
    // Trim description to 1 sensibly long line for the item view
    QString descLine = description;
    const int newLinePos = descLine.indexOf(QLatin1Char('\n'));
    if (newLinePos != -1)
        descLine.truncate(newLinePos);
    if (descLine.size() > MaxDescriptionLineLength) {
        const int dotPos = descLine.lastIndexOf(QLatin1Char('.'), MaxDescriptionLineLength);
        if (dotPos != -1)
            descLine.truncate(dotPos);
        else
            descLine.truncate(MaxDescriptionLineLength);
        descLine += QLatin1String("...");
    }
    items->at(descriptionColumn)->setText(descLine);
    // Set a HTML tooltip to make lines wrap and the markup sprinkled within work
    const QString htmlTip = QLatin1String("<html><body>") + description + QLatin1String("</body></html>");
    const int size = items->size();
    for (int i = 0; i < size; i++)
        items->at(i)->setToolTip(htmlTip);
    if (url) {
        // Should the text contain an URL, extract
        // Do not fall for "(http://XX)", strip special characters
        static QRegExp urlRegExp(QLatin1String("(http://[\\w\\.-]+/[a-zA-Z0-9/\\-&]*)"));
        QTC_CHECK(urlRegExp.isValid());
        if (urlRegExp.indexIn(description) != -1)
            *url= urlRegExp.cap(1);
        else
            url->clear();
    }
}

void GitoriousProjectWidget::grabFocus()
{
    ui->projectTreeView->setFocus();
}

void GitoriousProjectWidget::slotUpdateCheckBoxChanged(int state)
{
    if (state == Qt::Checked)
        slotUpdateProjects(Gitorious::instance().findByHostName(m_hostName));
}

void GitoriousProjectWidget::slotUpdateProjects(int hostIndex)
{
    if (!ui->updateCheckBox->isChecked())
        return;
    const Gitorious &gitorious = Gitorious::instance();
    // Complete list of projects
    if (m_hostName != gitorious.hostName(hostIndex))
        return;
    // Fill in missing projects
    const GitoriousHost::ProjectList &projects = gitorious.hosts().at(hostIndex).projects;
    const int size = projects.size();
    for (int i = m_model->rowCount(); i < size; i++)
        m_model->appendRow(projectEntry(*projects.at(i)));
    if (gitorious.hostState(hostIndex) == GitoriousHost::ProjectsComplete)
        ui->updateCheckBox->setVisible(false);
}

bool GitoriousProjectWidget::isValid() const
{
    return m_valid;
}

int GitoriousProjectWidget::hostIndex() const
{
    return Gitorious::instance().findByHostName(m_hostName);
}

QSharedPointer<GitoriousProject> GitoriousProjectWidget::project() const
{
    if (const QStandardItem *item = currentItem()) {
        const int projectIndex = item->row();
        return Gitorious::instance().hosts().at(hostIndex()).projects.at(projectIndex);
    }
    return QSharedPointer<GitoriousProject>(new GitoriousProject);
}

void GitoriousProjectWidget::changeEvent(QEvent *e)
{
    QWidget::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

} // namespace Internal
} // namespace Gitorious
