/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "promptoverwritedialog.h"

#include <utils/stringutils.h>

#include <QTreeView>
#include <QLabel>
#include <QStandardItemModel>
#include <QStandardItem>
#include <QDialogButtonBox>
#include <QVBoxLayout>

#include <QDir>

enum { FileNameRole = Qt::UserRole + 1 };

/*!
    \class Core::Internal::PromptOverwriteDialog
    \brief The PromptOverwriteDialog class implements a dialog that asks
    users whether they want to overwrite files.

    The dialog displays the common folder and the files in a list where users
    can select the files to overwrite.
*/

static inline QString fileNameOfItem(const QStandardItem *item)
{
    return item->data(FileNameRole).toString();
}

namespace Core {

PromptOverwriteDialog::PromptOverwriteDialog(QWidget *parent) :
    QDialog(parent),
    m_label(new QLabel),
    m_view(new QTreeView),
    m_model(new QStandardItemModel(0, 1, this))
{
    setWindowTitle(tr("Overwrite Existing Files"));
    setModal(true);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(m_label);
    m_view->setRootIsDecorated(false);
    m_view->setUniformRowHeights(true);
    m_view->setHeaderHidden(true);
    m_view->setSelectionMode(QAbstractItemView::NoSelection);
    m_view->setModel(m_model);
    mainLayout->addWidget(m_view);
    QDialogButtonBox *bb = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);
    connect(bb, SIGNAL(accepted()), this, SLOT(accept()));
    connect(bb, SIGNAL(rejected()), this, SLOT(reject()));
    mainLayout->addWidget(bb);
}

void PromptOverwriteDialog::setFiles(const QStringList &l)
{
    // Format checkable list excluding common path
    const QString nativeCommonPath = QDir::toNativeSeparators(Utils::commonPath(l));
    foreach (const QString &fileName, l) {
        const QString nativeFileName = QDir::toNativeSeparators(fileName);
        const int length = nativeFileName.size() - nativeCommonPath.size() - 1;
        QStandardItem *item = new QStandardItem(nativeFileName.right(length));
        item->setData(QVariant(fileName), FileNameRole);
        item->setFlags(Qt::ItemIsEnabled);
        item->setCheckable(true);
        item->setCheckState(Qt::Checked);
        m_model->appendRow(item);
    }
    const QString message =
        tr("The following files already exist in the folder\n%1.\n"
           "Would you like to overwrite them?").arg(nativeCommonPath);
    m_label->setText(message);
}

QStandardItem *PromptOverwriteDialog::itemForFile(const QString &f) const
{
    const int rowCount = m_model->rowCount();
    for (int r = 0; r < rowCount; ++r) {
        QStandardItem *item = m_model->item(r, 0);
        if (fileNameOfItem(item) == f)
            return item;
    }
    return 0;
}

QStringList PromptOverwriteDialog::files(Qt::CheckState cs) const
{
    QStringList result;
    const int rowCount = m_model->rowCount();
    for (int r = 0; r < rowCount; ++r) {
        const QStandardItem *item = m_model->item(r, 0);
        if (item->checkState() == cs)
            result.push_back(fileNameOfItem(item));
    }
    return result;
}

void PromptOverwriteDialog::setFileEnabled(const QString &f, bool e)
{
    if (QStandardItem *item = itemForFile(f)) {
        Qt::ItemFlags flags = item->flags();
        if (e)
            flags |= Qt::ItemIsEnabled;
        else
            flags &= ~Qt::ItemIsEnabled;
        item->setFlags(flags);
    }
}

bool PromptOverwriteDialog::isFileEnabled(const QString &f) const
{
    if (const QStandardItem *item = itemForFile(f))
        return (item->flags() & Qt::ItemIsEnabled);
    return false;
}

void PromptOverwriteDialog::setFileChecked(const QString &f, bool e)
{
    if (QStandardItem *item = itemForFile(f))
        item->setCheckState(e ? Qt::Checked : Qt::Unchecked);
}

bool PromptOverwriteDialog::isFileChecked(const QString &f) const
{
    if (const QStandardItem *item = itemForFile(f))
        return item->checkState() == Qt::Checked;
    return false;
}

} // namespace Core
