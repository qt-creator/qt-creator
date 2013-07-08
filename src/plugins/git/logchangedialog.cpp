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

#include "logchangedialog.h"
#include "gitplugin.h"
#include "gitclient.h"

#include <vcsbase/vcsbaseoutputwindow.h>

#include <QTreeView>
#include <QLabel>
#include <QPushButton>
#include <QStandardItemModel>
#include <QDialogButtonBox>
#include <QItemSelectionModel>
#include <QVBoxLayout>
#include <QComboBox>

namespace Git {
namespace Internal {

enum Columns
{
    Sha1Column,
    SubjectColumn,
    ColumnCount
};

LogChangeWidget::LogChangeWidget(QWidget *parent)
    : QTreeView(parent)
    , m_model(new QStandardItemModel(0, ColumnCount, this))
{
    QStringList headers;
    headers << tr("Sha1")<< tr("Subject");
    m_model->setHorizontalHeaderLabels(headers);
    setModel(m_model);
    setMinimumWidth(300);
    setUniformRowHeights(true);
    setRootIsDecorated(false);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    connect(this, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(emitDoubleClicked(QModelIndex)));
}

bool LogChangeWidget::init(const QString &repository, const QString &commit, bool includeRemote)
{
    if (!populateLog(repository, commit, includeRemote))
        return false;
    if (!m_model->rowCount()) {
        VcsBase::VcsBaseOutputWindow::instance()->appendError(
                    GitPlugin::instance()->gitClient()->msgNoCommits(includeRemote));
        return false;
    }
    return true;
}

QString LogChangeWidget::commit() const
{
    if (const QStandardItem *sha1Item = currentItem(Sha1Column))
        return sha1Item->text();
    return QString();
}

int LogChangeWidget::commitIndex() const
{
    const QModelIndex currentIndex = selectionModel()->currentIndex();
    if (currentIndex.isValid())
        return currentIndex.row();
    return -1;
}

QString LogChangeWidget::earliestCommit() const
{
    int rows = m_model->rowCount();
    if (rows) {
        if (const QStandardItem *item = m_model->item(rows - 1, Sha1Column))
            return item->text();
    }
    return QString();
}

void LogChangeWidget::emitDoubleClicked(const QModelIndex &index)
{
    if (index.isValid()) {
        QString commit = index.sibling(index.row(), Sha1Column).data().toString();
        if (!commit.isEmpty())
            emit doubleClicked(commit);
    }
}

bool LogChangeWidget::populateLog(const QString &repository, const QString &commit, bool includeRemote)
{
    const QString currentCommit = this->commit();
    int selected = currentCommit.isEmpty() ? 0 : -1;
    if (const int rowCount = m_model->rowCount())
        m_model->removeRows(0, rowCount);

    // Retrieve log using a custom format "Sha1:Subject [(refs)]"
    GitClient *client = GitPlugin::instance()->gitClient();
    QStringList arguments;
    arguments << QLatin1String("--max-count=40") << QLatin1String("--format=%h:%s %d");
    arguments << (commit.isEmpty() ? QLatin1String("HEAD") : commit);
    if (!includeRemote)
        arguments << QLatin1String("--not") << QLatin1String("--remotes");
    QString output;
    if (!client->synchronousLog(repository, arguments, &output))
        return false;
    foreach (const QString &line, output.split(QLatin1Char('\n'))) {
        const int colonPos = line.indexOf(QLatin1Char(':'));
        if (colonPos != -1) {
            QList<QStandardItem *> row;
            for (int c = 0; c < ColumnCount; ++c) {
                QStandardItem *item = new QStandardItem;
                item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
                if (line.endsWith(QLatin1Char(')'))) {
                    QFont font = item->font();
                    font.setBold(true);
                    item->setFont(font);
                }
                row.push_back(item);
            }
            const QString sha1 = line.left(colonPos);
            row[Sha1Column]->setText(sha1);
            row[SubjectColumn]->setText(line.right(line.size() - colonPos - 1));
            m_model->appendRow(row);
            if (selected == -1 && currentCommit == sha1)
                selected = m_model->rowCount() - 1;
        }
    }
    setCurrentIndex(m_model->index(selected, 0));
    return true;
}

const QStandardItem *LogChangeWidget::currentItem(int column) const
{
    const QModelIndex currentIndex = selectionModel()->currentIndex();
    if (currentIndex.isValid())
        return m_model->item(currentIndex.row(), column);
    return 0;
}

LogChangeDialog::LogChangeDialog(bool isReset, QWidget *parent) :
    QDialog(parent)
    , widget(new LogChangeWidget)
    , m_dialogButtonBox(new QDialogButtonBox(this))
    , m_resetTypeComboBox(0)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(new QLabel(isReset ? tr("Reset to:") : tr("Select change:"), this));
    layout->addWidget(widget);
    QHBoxLayout *popUpLayout = new QHBoxLayout;
    if (isReset) {
        popUpLayout->addWidget(new QLabel(tr("Reset type:"), this));
        m_resetTypeComboBox = new QComboBox(this);
        m_resetTypeComboBox->addItem(tr("Hard"), QLatin1String("--hard"));
        m_resetTypeComboBox->addItem(tr("Mixed"), QLatin1String("--mixed"));
        m_resetTypeComboBox->addItem(tr("Soft"), QLatin1String("--soft"));
        popUpLayout->addWidget(m_resetTypeComboBox);
        popUpLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Ignored));
    }

    popUpLayout->addWidget(m_dialogButtonBox);
    m_dialogButtonBox->addButton(QDialogButtonBox::Cancel);
    QPushButton *okButton = m_dialogButtonBox->addButton(QDialogButtonBox::Ok);
    layout->addLayout(popUpLayout);

    connect(m_dialogButtonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(m_dialogButtonBox, SIGNAL(rejected()), this, SLOT(reject()));

    connect(widget, SIGNAL(doubleClicked(QModelIndex)), okButton, SLOT(animateClick()));

    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    resize(600, 400);
}

bool LogChangeDialog::runDialog(const QString &repository, const QString &commit, bool includeRemote)
{
    if (!widget->init(repository, commit, includeRemote))
        return false;

    return QDialog::exec() == QDialog::Accepted;
}

QString LogChangeDialog::commit() const
{
    return widget->commit();
}

int LogChangeDialog::commitIndex() const
{
    return widget->commitIndex();
}

QString LogChangeDialog::resetFlag() const
{
    if (!m_resetTypeComboBox)
        return QString();
    return m_resetTypeComboBox->itemData(m_resetTypeComboBox->currentIndex()).toString();
}

} // namespace Internal
} // namespace Git
