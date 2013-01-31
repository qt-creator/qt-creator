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

#include "gitorious.h"
#include "gitorioushostwidget.h"
#include "ui_gitorioushostwidget.h"

#include <coreplugin/coreconstants.h>

#include <QUrl>
#include <QDebug>
#include <QTimer>

#include <QStandardItem>
#include <QStandardItemModel>
#include <QItemSelectionModel>
#include <QDesktopServices>
#include <QIcon>
#include <QStyle>

enum { debug = 0 };

namespace Gitorious {
namespace Internal {

enum { HostNameColumn, ProjectCountColumn, DescriptionColumn, ColumnCount };

// Create a model row for a host. Make the host name editable as specified by
// flag.
static QList<QStandardItem *> hostEntry(const QString &host,
                                        int projectCount,
                                        const QString &description, bool isDummyEntry)
{
    const Qt::ItemFlags nonEditableFlags = (Qt::ItemIsSelectable|Qt::ItemIsEnabled);
    const Qt::ItemFlags editableFlags = nonEditableFlags|Qt::ItemIsEditable;
    QStandardItem *hostItem = new QStandardItem(host);
    hostItem->setFlags(isDummyEntry ? editableFlags : nonEditableFlags);
    // Empty for dummy, else "..." or count
    QStandardItem *projectCountItem = 0;
    QString countItemText;
    if (!isDummyEntry)
        countItemText = projectCount ? QString::number(projectCount) : QString(QLatin1String("..."));
    projectCountItem = new QStandardItem(countItemText);
    projectCountItem->setFlags(nonEditableFlags);
    QStandardItem *descriptionItem = new QStandardItem(description);
    descriptionItem->setFlags(editableFlags);
    QList<QStandardItem *> rc;
    rc << hostItem << projectCountItem << descriptionItem;
    return rc;
}

static inline QList<QStandardItem *> hostEntry(const GitoriousHost &h)
{
    return hostEntry(h.hostName, h.projects.size(), h.description, false);
}

GitoriousHostWidget::GitoriousHostWidget(QWidget *parent) :
    QWidget(parent),
    m_newHost(tr("<New Host>")),
    ui(new Ui::GitoriousHostWidget),
    m_model(new QStandardItemModel(0, ColumnCount)),
    m_errorClearTimer(0),
    m_isValid(false),
    m_isHostListDirty(false)
{
    ui->setupUi(this);
    ui->errorLabel->setVisible(false);
    ui->browseToolButton->setIcon(style()->standardIcon(QStyle::SP_MessageBoxInformation));
    connect(ui->browseToolButton, SIGNAL(clicked()), this, SLOT(slotBrowse()));
    ui->browseToolButton->setEnabled(false);
    ui->deleteToolButton->setIcon(QIcon(QLatin1String(Core::Constants::ICON_MINUS)));
    connect(ui->deleteToolButton, SIGNAL(clicked()), this, SLOT(slotDelete()));
    ui->deleteToolButton->setEnabled(false);

    // Model
    QStringList headers;
    headers << tr("Host") << tr("Projects") << tr("Description");
    m_model->setHorizontalHeaderLabels(headers);

    Gitorious &gitorious = Gitorious::instance();
    foreach (const GitoriousHost &gh, gitorious.hosts())
        m_model->appendRow(hostEntry(gh));
    appendNewDummyEntry();
    connect(m_model, SIGNAL(itemChanged(QStandardItem*)), this, SLOT(slotItemEdited(QStandardItem*)));
    ui->hostView->setModel(m_model);

    // View
    ui->hostView->setRootIsDecorated(false);
    ui->hostView->setUniformRowHeights(true);
    connect(ui->hostView->selectionModel(), SIGNAL(currentChanged(QModelIndex,QModelIndex)),
            this, SLOT(slotCurrentChanged(QModelIndex,QModelIndex)));

    ui->hostView->setSelectionMode(QAbstractItemView::SingleSelection);
    if (m_model->rowCount())
        selectRow(0);

    connect(&gitorious, SIGNAL(projectListPageReceived(int,int)),
            this, SLOT(slotProjectListPageReceived(int)));
    connect(&gitorious, SIGNAL(projectListReceived(int)),
            this, SLOT(slotProjectListPageReceived(int)));

    connect(&gitorious, SIGNAL(error(QString)), this, SLOT(slotError(QString)));

    setMinimumWidth(700);
}

GitoriousHostWidget::~GitoriousHostWidget()
{
    // Prevent crash?
    Gitorious *gitorious = &Gitorious::instance();
    disconnect(gitorious, SIGNAL(projectListPageReceived(int,int)),
               this, SLOT(slotProjectListPageReceived(int)));
    disconnect(gitorious, SIGNAL(projectListReceived(int)),
               this, SLOT(slotProjectListPageReceived(int)));
    disconnect(gitorious, SIGNAL(error(QString)), this, SLOT(slotError(QString)));
    delete ui;
}

int GitoriousHostWidget::selectedRow() const
{
    const QModelIndex idx = ui->hostView->selectionModel()->currentIndex();
    if (idx.isValid())
        return idx.row();
    return -1;
}

void GitoriousHostWidget::selectRow(int r)
{
    if (r >= 0 && r != selectedRow()) {
        const QModelIndex index = m_model->index(r, 0);
        ui->hostView->selectionModel()->setCurrentIndex(index, QItemSelectionModel::Select|QItemSelectionModel::Current|QItemSelectionModel::Rows);
    }
}

void GitoriousHostWidget::appendNewDummyEntry()
{
    // Append a new entry where a host name is editable
    m_model->appendRow(hostEntry(m_newHost, 0, QString(), true));
}

void GitoriousHostWidget::slotItemEdited(QStandardItem *item)
{
    // Synchronize with Gitorious singleton.
    // Did someone enter a valid host name into the dummy item?
    // -> Create a new one.
    const int row = item->row();
    const bool isDummyEntry = row >= Gitorious::instance().hostCount();
    switch (item->column()) {
    case HostNameColumn:
        if (isDummyEntry) {
            Gitorious::instance().addHost(item->text(), m_model->item(row, DescriptionColumn)->text());
            m_isHostListDirty = true;
            appendNewDummyEntry();
            selectRow(row);
        }
        break;
    case ProjectCountColumn:
        break;
    case DescriptionColumn:
        if (!isDummyEntry) {
            const QString description = item->text();
            if (description != Gitorious::instance().hostDescription(row)) {
                Gitorious::instance().setHostDescription(row, item->text());
                m_isHostListDirty = true;
            }
        }
        break;
    }
}

void GitoriousHostWidget::slotProjectListPageReceived(int row)
{
    if (debug)
        qDebug() << Q_FUNC_INFO << row;
    // Update column
    const int projectCount = Gitorious::instance().projectCount(row);
    m_model->item(row, ProjectCountColumn)->setText(QString::number(projectCount));
    // If it is the currently selected host, re-check validity if not enabled
    if (!m_isValid) {
        const QModelIndex current = ui->hostView->selectionModel()->currentIndex();
        if (current.isValid() && current.row() == row)
            checkValid(current);
    }
}

QStandardItem *GitoriousHostWidget::currentItem() const
{
    const QModelIndex idx = ui->hostView->selectionModel()->currentIndex();
    if (idx.isValid())
        return m_model->itemFromIndex(idx.column() != 0 ? idx.sibling(idx.row(), 0) : idx);
    return 0;
}

void GitoriousHostWidget::slotBrowse()
{
    if (const QStandardItem *item = currentItem()) {
        const QUrl url(QLatin1String("http://") + item->text() + QLatin1Char('/'));
        if (url.isValid())
            QDesktopServices::openUrl(url);
    }
}

void GitoriousHostWidget::slotDelete()
{
    const QModelIndex index = ui->hostView->selectionModel()->currentIndex();
    ui->hostView->selectionModel()->clear();
    if (index.isValid()) {
        const int row = index.row();
        qDeleteAll(m_model->takeRow(row));
        Gitorious::instance().removeAt(row);
        m_isHostListDirty = true;
    }
}

void GitoriousHostWidget::slotCurrentChanged(const QModelIndex &current, const QModelIndex & /* previous */)
{
    checkValid(current);
}

void GitoriousHostWidget::checkValid(const QModelIndex &index)
{
    if (debug)
        qDebug() << Q_FUNC_INFO << index;
    bool hasSelectedHost = false;
    bool hasProjects = false;
    if (index.isValid()) {
        // Are we on the new dummy item?
        Gitorious &gitorious = Gitorious::instance();
        const int row = index.row();
        hasSelectedHost = row < gitorious.hostCount();
        hasProjects = hasSelectedHost && gitorious.projectCount(row) > 0;
    }
    ui->deleteToolButton->setEnabled(hasSelectedHost);
    ui->browseToolButton->setEnabled(hasSelectedHost);

    const bool valid = hasSelectedHost && hasProjects;
    if (valid != m_isValid) {
        m_isValid = valid;
        emit validChanged();
    }
}

bool GitoriousHostWidget::isValid() const
{
    return m_isValid;
}

bool GitoriousHostWidget::isHostListDirty() const
{
    return m_isHostListDirty;
}

void GitoriousHostWidget::slotClearError()
{
    ui->errorLabel->setVisible(false);
    ui->errorLabel->clear();
}

void GitoriousHostWidget::slotError(const QString &e)
{
    // Display error for a while
    ui->errorLabel->setText(e);
    ui->errorLabel->setVisible(true);
    if (!m_errorClearTimer) {
        m_errorClearTimer = new QTimer(this);
        m_errorClearTimer->setSingleShot(true);
        m_errorClearTimer->setInterval(5000);
        connect(m_errorClearTimer, SIGNAL(timeout()), this, SLOT(slotClearError()));
    }
    if (!m_errorClearTimer->isActive())
        m_errorClearTimer->start();
}

void GitoriousHostWidget::changeEvent(QEvent *e)
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
