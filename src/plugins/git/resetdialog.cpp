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

#include "resetdialog.h"
#include "gitplugin.h"
#include "gitclient.h"

#include <QTreeView>
#include <QLabel>
#include <QPushButton>
#include <QStandardItemModel>
#include <QDialogButtonBox>
#include <QItemSelectionModel>
#include <QVBoxLayout>
#include <QComboBox>
#include <QDir>

namespace Git {
namespace Internal {

enum Columns
{
    Sha1Column,
    SubjectColumn,
    ColumnCount
};

ResetDialog::ResetDialog(QWidget *parent)
    : QDialog(parent)
    , m_treeView(new QTreeView(this))
    , m_model(new QStandardItemModel(0, ColumnCount, this))
    , m_dialogButtonBox(new QDialogButtonBox(this))
    , m_resetTypeComboBox(new QComboBox(this))
{
    QStringList headers;
    headers << tr("Sha1")<< tr("Subject");
    m_model->setHorizontalHeaderLabels(headers);
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(new QLabel(tr("Reset to:"), this));
    m_treeView->setModel(m_model);
    m_treeView->setMinimumWidth(300);
    m_treeView->setUniformRowHeights(true);
    m_treeView->setRootIsDecorated(false);
    m_treeView->setSelectionBehavior(QAbstractItemView::SelectRows);
    layout->addWidget(m_treeView);
    QHBoxLayout *popUpLayout = new QHBoxLayout();
    popUpLayout->addWidget(new QLabel(tr("Reset type:"), this));
    m_resetTypeComboBox->addItem(tr("Hard Reset"), HardReset);
    m_resetTypeComboBox->addItem(tr("Soft Reset"), SoftReset);
    popUpLayout->addWidget(m_resetTypeComboBox);
    popUpLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Ignored));

    popUpLayout->addWidget(m_dialogButtonBox);
    m_dialogButtonBox->addButton(QDialogButtonBox::Cancel);
    QPushButton *okButton = m_dialogButtonBox->addButton(QDialogButtonBox::Ok);
    layout->addLayout(popUpLayout);

    connect(m_treeView, SIGNAL(doubleClicked(QModelIndex)),
            okButton, SLOT(animateClick()));

    connect(m_dialogButtonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(m_dialogButtonBox, SIGNAL(rejected()), this, SLOT(reject()));

    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    resize(600, 400);
}

bool ResetDialog::runDialog(const QString &repository)
{
    setWindowTitle(tr("Undo Changes to %1").arg(QDir::toNativeSeparators(repository)));

    if (!populateLog(repository) || !m_model->rowCount())
        return QDialog::Rejected;

    m_treeView->selectionModel()->select(m_model->index(0, 0),
                                         QItemSelectionModel::SelectCurrent | QItemSelectionModel::Rows);

    return exec() == QDialog::Accepted;
}

QString ResetDialog::commit() const
{
    // Return Sha1, or empty for top commit.
    if (const QStandardItem *sha1Item = currentItem(Sha1Column))
        return sha1Item->row() ? sha1Item->text() :  QString();
    return QString();
}

ResetType ResetDialog::resetType() const
{
    return static_cast<ResetType>(m_resetTypeComboBox->itemData(m_resetTypeComboBox->currentIndex()).toInt());
}

bool ResetDialog::populateLog(const QString &repository)
{
    if (const int rowCount = m_model->rowCount())
        m_model->removeRows(0, rowCount);

    // Retrieve log using a custom format "Sha1:Subject"
    GitClient *client = GitPlugin::instance()->gitClient();
    QStringList arguments;
    arguments << QLatin1String("--max-count=30") << QLatin1String("--format=%h:%s");
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
                row.push_back(item);
            }
            row[Sha1Column]->setText(line.left(colonPos));
            row[SubjectColumn]->setText(line.right(line.size() - colonPos - 1));
            m_model->appendRow(row);
        }
    }
    return true;
}

const QStandardItem *ResetDialog::currentItem(int column) const
{
    const QModelIndex currentIndex = m_treeView->selectionModel()->currentIndex();
    if (currentIndex.isValid())
        return m_model->item(currentIndex.row(), column);
    return 0;
}

} // namespace Internal
} // namespace Git
