// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "addtovcsdialog.h"

#include "../coreplugintr.h"

#include <utils/layoutbuilder.h>

#include <QDialogButtonBox>
#include <QDir>
#include <QListWidget>
#include <QListWidgetItem>
#include <QScrollArea>

namespace Core {
namespace Internal {

AddToVcsDialog::AddToVcsDialog(QWidget *parent,
                               const QString &title,
                               const Utils::FilePaths &files,
                               const QString &vcsDisplayName)
    : QDialog(parent)
{
    using namespace Layouting;

    resize(363, 375);
    setMinimumSize({200, 200});
    setBaseSize({300, 500});
    setWindowTitle(title);

    auto filesListWidget = new QListWidget;
    filesListWidget->setSelectionMode(QAbstractItemView::NoSelection);
    filesListWidget->setSelectionBehavior(QAbstractItemView::SelectRows);

    QWidget *scrollAreaWidgetContents = Column{filesListWidget, noMargin}.emerge();
    scrollAreaWidgetContents->setGeometry({0, 0, 341, 300});

    auto scrollArea = new QScrollArea;
    scrollArea->setWidgetResizable(true);
    scrollArea->setWidget(scrollAreaWidgetContents);

    auto buttonBox = new QDialogButtonBox;
    buttonBox->setStandardButtons(QDialogButtonBox::No | QDialogButtonBox::Yes);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    const QString addTo = files.size() == 1
                              ? Tr::tr("Add the file to version control (%1)").arg(vcsDisplayName)
                              : Tr::tr("Add the files to version control (%1)").arg(vcsDisplayName);

    // clang-format off
    Column {
        addTo,
        scrollArea,
        buttonBox
    }.attachTo(this);
    // clang-format on

    for (const Utils::FilePath &file : files) {
        QListWidgetItem *item = new QListWidgetItem(file.toUserOutput());
        filesListWidget->addItem(item);
    }
}

AddToVcsDialog::~AddToVcsDialog() = default;

} // namespace Internal
} // namespace Core
