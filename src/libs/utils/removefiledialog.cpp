// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "removefiledialog.h"

#include "filepath.h"
#include "layoutbuilder.h"

#include <QApplication>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QLabel>

namespace Utils {

RemoveFileDialog::RemoveFileDialog(const FilePath &filePath, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Remove File"));
    resize(514, 159);

    QFont font;
    font.setFamilies({"Courier New"});

    auto fileNameLabel = new QLabel(filePath.toUserOutput());
    fileNameLabel->setFont(font);
    fileNameLabel->setWordWrap(true);

    m_deleteFileCheckBox = new QCheckBox(tr("&Delete file permanently"));

    auto removeVCCheckBox = new QCheckBox(tr("&Remove from version control"));
    removeVCCheckBox->setVisible(false); // TODO

    auto buttonBox = new QDialogButtonBox(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

    using namespace Layouting;

    Column {
        tr("File to remove:"),
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
