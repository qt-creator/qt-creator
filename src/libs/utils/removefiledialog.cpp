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
