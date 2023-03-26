// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "removefiledialog.h"

#include "filepath.h"
#include "layoutbuilder.h"
#include "utilstr.h"

#include <QApplication>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QLabel>

namespace Utils {

RemoveFileDialog::RemoveFileDialog(const FilePath &filePath, QWidget *parent)
    : QDialog(parent)
{
    const bool isFile = filePath.isFile();
    setWindowTitle(isFile ? Tr::tr("Remove File") : Tr::tr("Remove Folder"));
    resize(514, 159);

    QFont font;
    font.setFamilies({"Courier New"});

    auto fileNameLabel = new QLabel(filePath.toUserOutput());
    fileNameLabel->setFont(font);
    fileNameLabel->setWordWrap(true);

    m_deleteFileCheckBox = new QCheckBox(Tr::tr("&Delete file permanently"));

    auto removeVCCheckBox = new QCheckBox(Tr::tr("&Remove from version control"));
    removeVCCheckBox->setVisible(false); // TODO

    auto buttonBox = new QDialogButtonBox(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

    using namespace Layouting;

    Column {
        isFile ? Tr::tr("File to remove:") : Tr::tr("Folder to remove:"),
        fileNameLabel,
        Space(10),
        m_deleteFileCheckBox,
        removeVCCheckBox,
        buttonBox
    }.attachTo(this);

    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

RemoveFileDialog::~RemoveFileDialog() = default;

void RemoveFileDialog::setDeleteFileVisible(bool visible)
{
    m_deleteFileCheckBox->setVisible(visible);
}

bool RemoveFileDialog::isDeleteFileChecked() const
{
    return m_deleteFileCheckBox->isChecked();
}

} // Utils
