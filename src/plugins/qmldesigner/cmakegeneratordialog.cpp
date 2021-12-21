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
#include "cmakegeneratordialogtreemodel.h"
#include "generatecmakelistsconstants.h"

#include <QDialogButtonBox>
#include <QPushButton>
#include <QLayout>
#include <QLabel>
#include <QTreeView>

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

    m_model = new CMakeGeneratorDialogTreeModel(rootDir, files, this);

    QTreeView *tree = new QTreeView(this);
    tree->setModel(m_model);
    tree->expandAll();
    tree->setHeaderHidden(true);
    tree->setItemsExpandable(false);

    layout->addWidget(tree);
    layout->addWidget(buttons);
}

FilePaths CmakeGeneratorDialog::getFilePaths()
{
    FilePaths paths;

    QList<CheckableFileTreeItem*> items = m_model->checkedItems();
    for (CheckableFileTreeItem *item: items) {
        paths.append(FilePath::fromString(item->text()));
    }

    return paths;
}

}
}
