/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "addtovcsdialog.h"

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
    using namespace Utils::Layouting;

    resize(363, 375);
    setMinimumSize({200, 200});
    setBaseSize({300, 500});
    setWindowTitle(title);

    auto filesListWidget = new QListWidget;
    filesListWidget->setSelectionMode(QAbstractItemView::NoSelection);
    filesListWidget->setSelectionBehavior(QAbstractItemView::SelectRows);

    QWidget *scrollAreaWidgetContents = Column{filesListWidget}.emerge(WithoutMargins);
    scrollAreaWidgetContents->setGeometry({0, 0, 341, 300});

    auto scrollArea = new QScrollArea;
    scrollArea->setWidgetResizable(true);
    scrollArea->setWidget(scrollAreaWidgetContents);

    auto buttonBox = new QDialogButtonBox;
    buttonBox->setStandardButtons(QDialogButtonBox::No | QDialogButtonBox::Yes);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    const QString addTo = files.size() == 1
                              ? tr("Add the file to version control (%1)").arg(vcsDisplayName)
                              : tr("Add the files to version control (%1)").arg(vcsDisplayName);

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
