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

#include <utils/utilsicons.h>

#include <QDialogButtonBox>
#include <QPushButton>
#include <QLayout>
#include <QLabel>
#include <QTreeView>

using namespace Utils;

namespace QmlDesigner {
namespace GenerateCmake {

CmakeGeneratorDialog::CmakeGeneratorDialog(const FilePath &rootDir, const FilePaths &files)
    : QDialog(),
      m_rootDir(rootDir),
      m_files(files)
{
    setWindowTitle(QCoreApplication::translate("QmlDesigner::GenerateCmake",
                                               "Select Files to Generate"));

    m_model = new CMakeGeneratorDialogTreeModel(rootDir, files, this);

    QVBoxLayout *layout = new QVBoxLayout(this);
    setLayout(layout);

    QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);

    auto *okButton = buttons->button(QDialogButtonBox::Ok);
    okButton->setDefault(true);

    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(m_model, &CMakeGeneratorDialogTreeModel::checkedStateChanged, this, &CmakeGeneratorDialog::refreshNotificationText);

    QTreeView *tree = new QTreeView(this);
    tree->setModel(m_model);
    tree->expandAll();
    tree->setHeaderHidden(true);

    m_notifications = new QTextEdit(this);
    m_warningIcon = Utils::Icons::WARNING.pixmap();

    refreshNotificationText();

    layout->addWidget(tree, 2);
    layout->addWidget(m_notifications, 1);
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

const QString FILE_CREATE_NOTIFICATION = QCoreApplication::translate("QmlDesigner::GenerateCmake",
                                                                    "File %1 will be created.\n");
const QString FILE_OVERWRITE_NOTIFICATION = QCoreApplication::translate("QmlDesigner::GenerateCmake",
                                                                       "File %1 will be overwritten.\n");

void CmakeGeneratorDialog::refreshNotificationText()
{
    QTextDocument *document = m_notifications->document();
    document->clear();
    document->addResource(QTextDocument::ImageResource, QUrl("cmakegendialog://warningicon"), m_warningIcon);

    QTextCursor cursor = m_notifications->textCursor();
    QTextImageFormat iformat;
    iformat.setName("cmakegendialog://warningicon");

    QList<CheckableFileTreeItem*> nodes = m_model->items();

    for (CheckableFileTreeItem *node : nodes) {
        if (!m_files.contains(node->toFilePath()))
            continue;

        if (!node->toFilePath().exists() && node->isChecked()) {
            QString relativePath = QString(node->toFilePath().toString()).remove(m_rootDir.toString()+'/');
            cursor.insertText(QString(FILE_CREATE_NOTIFICATION).arg(relativePath));
        }
    }

    if (!document->toPlainText().isEmpty())
        cursor.insertBlock();

    for (CheckableFileTreeItem *node : nodes) {
        if (!m_files.contains(node->toFilePath()))
            continue;

        if (node->toFilePath().exists() && node->isChecked()) {
            QString relativePath = node->toFilePath().relativePath(m_rootDir).toString();
            cursor.insertImage(iformat);
            cursor.insertText(QString(FILE_OVERWRITE_NOTIFICATION).arg(relativePath));
        }
    }
}

}
}
