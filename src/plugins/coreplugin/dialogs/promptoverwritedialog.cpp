// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "promptoverwritedialog.h"

#include "../coreplugintr.h"

#include <utils/fileutils.h>
#include <utils/stringutils.h>

#include <QTreeView>
#include <QLabel>
#include <QStandardItemModel>
#include <QStandardItem>
#include <QDialogButtonBox>
#include <QVBoxLayout>

#include <QDir>

enum { FileNameRole = Qt::UserRole + 1 };

using namespace Utils;

namespace Core {

namespace Internal {
class PromptOverwriteDialogPrivate
{
public:
    PromptOverwriteDialogPrivate(PromptOverwriteDialog *dialog)
        : m_label(new QLabel)
        , m_view(new QTreeView)
        , m_model(new QStandardItemModel(0, 1, dialog))
    {}
    QLabel *m_label;
    QTreeView *m_view;
    QStandardItemModel *m_model;
};
} // namespace Internal

static FilePath fileNameOfItem(const QStandardItem *item)
{
    return FilePath::fromString(item->data(FileNameRole).toString());
}

/*!
    \class Core::PromptOverwriteDialog
    \inmodule QtCreator
    \internal
    \brief The PromptOverwriteDialog class implements a dialog that asks
    users whether they want to overwrite files.

    The dialog displays the common folder and the files in a list where users
    can select the files to overwrite.
*/

PromptOverwriteDialog::PromptOverwriteDialog(QWidget *parent)
    : QDialog(parent)
    , d(new Internal::PromptOverwriteDialogPrivate(this))
{
    setWindowTitle(Tr::tr("Overwrite Existing Files"));
    setModal(true);
    auto mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(d->m_label);
    d->m_view->setRootIsDecorated(false);
    d->m_view->setUniformRowHeights(true);
    d->m_view->setHeaderHidden(true);
    d->m_view->setSelectionMode(QAbstractItemView::NoSelection);
    d->m_view->setModel(d->m_model);
    mainLayout->addWidget(d->m_view);
    QDialogButtonBox *bb = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);
    connect(bb, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(bb, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(bb);
}

PromptOverwriteDialog::~PromptOverwriteDialog() = default;

void PromptOverwriteDialog::setFiles(const FilePaths &l)
{
    // Format checkable list excluding common path
    const QString nativeCommonPath = FileUtils::commonPath(l).toUserOutput();
    for (const FilePath &fileName : l) {
        const QString nativeFileName = fileName.toUserOutput();
        const int length = nativeFileName.size() - nativeCommonPath.size() - 1;
        QStandardItem *item = new QStandardItem(nativeFileName.right(length));
        item->setData(QVariant(fileName.toString()), FileNameRole);
        item->setFlags(Qt::ItemIsEnabled);
        item->setCheckable(true);
        item->setCheckState(Qt::Checked);
        d->m_model->appendRow(item);
    }
    const QString message =
        Tr::tr("The following files already exist in the folder\n%1.\n"
               "Would you like to overwrite them?").arg(nativeCommonPath);
    d->m_label->setText(message);
}

QStandardItem *PromptOverwriteDialog::itemForFile(const FilePath &f) const
{
    const int rowCount = d->m_model->rowCount();
    for (int r = 0; r < rowCount; ++r) {
        QStandardItem *item = d->m_model->item(r, 0);
        if (fileNameOfItem(item) == f)
            return item;
    }
    return nullptr;
}

FilePaths PromptOverwriteDialog::files(Qt::CheckState cs) const
{
    FilePaths result;
    const int rowCount = d->m_model->rowCount();
    for (int r = 0; r < rowCount; ++r) {
        const QStandardItem *item = d->m_model->item(r, 0);
        if (item->checkState() == cs)
            result.push_back(fileNameOfItem(item));
    }
    return result;
}

void PromptOverwriteDialog::setFileEnabled(const FilePath &f, bool e)
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

bool PromptOverwriteDialog::isFileEnabled(const FilePath &f) const
{
    if (const QStandardItem *item = itemForFile(f))
        return (item->flags() & Qt::ItemIsEnabled);
    return false;
}

void PromptOverwriteDialog::setFileChecked(const FilePath &f, bool e)
{
    if (QStandardItem *item = itemForFile(f))
        item->setCheckState(e ? Qt::Checked : Qt::Unchecked);
}

bool PromptOverwriteDialog::isFileChecked(const FilePath &f) const
{
    if (const QStandardItem *item = itemForFile(f))
        return item->checkState() == Qt::Checked;
    return false;
}

} // Core
