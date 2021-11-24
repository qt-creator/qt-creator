/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Design Tooling
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

#include "cmakegeneratordialog.h"
#include "generatecmakelistsconstants.h"

#include <QDialogButtonBox>
#include <QPushButton>
#include <QLayout>
#include <QLabel>
#include <QListView>

using namespace Utils;

namespace QmlDesigner {
namespace GenerateCmake {

CmakeGeneratorDialog::CmakeGeneratorDialog(const FilePath &rootDir, const FilePaths &files)
    : QDialog()
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    setLayout(layout);

    QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);

    auto *okButton = buttons->button(QDialogButtonBox::Ok);
    okButton->setDefault(true);

    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    model = new CMakeGeneratorDialogModel(rootDir, files, this);

    QListView *list = new QListView(this);
    list->setModel(model);

    layout->addWidget(list);
    layout->addWidget(buttons);
}

FilePaths CmakeGeneratorDialog::getFilePaths()
{
    FilePaths paths;

    QList<CheckableStandardItem*> items = model->checkedItems();
    for (CheckableStandardItem *item: items) {
        paths.append(FilePath::fromString(item->text()));
    }

    return paths;
}

CMakeGeneratorDialogModel::CMakeGeneratorDialogModel(const Utils::FilePath &rootDir, const Utils::FilePaths &files, QObject *parent)
    :CheckableFileListModel(rootDir, files, parent)
{
    for (int i=0; i<rowCount(); i++) {
        CheckableStandardItem *item = static_cast<CheckableStandardItem*>(QStandardItemModel::item(i));
        item->setChecked(CMakeGeneratorDialogModel::checkedByDefault(FilePath::fromString(item->text())));
    }
}

bool CMakeGeneratorDialogModel::checkedByDefault(const FilePath &path) const
{
    if (path.exists()) {
        QString relativePath = path.relativeChildPath(rootDir).toString();
        if (relativePath.compare(QmlDesigner::GenerateCmake::Constants::FILENAME_CMAKELISTS) == 0)
            return false;
        if (relativePath.endsWith(QmlDesigner::GenerateCmake::Constants::FILENAME_CMAKELISTS)
            && relativePath.length() > QString(QmlDesigner::GenerateCmake::Constants::FILENAME_CMAKELISTS).length())
            return true;
        if (relativePath.compare(QmlDesigner::GenerateCmake::Constants::FILENAME_MODULES) == 0)
            return true;
        if (relativePath.compare(
                FilePath::fromString(QmlDesigner::GenerateCmake::Constants::DIRNAME_CPP)
                .pathAppended(QmlDesigner::GenerateCmake::Constants::FILENAME_MAINCPP_HEADER)
                .toString())
                == 0)
            return true;
    }

    return !path.exists();
}

}
}
