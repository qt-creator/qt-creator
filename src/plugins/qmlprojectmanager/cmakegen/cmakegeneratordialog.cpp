// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cmakegeneratordialog.h"
#include "../qmlprojectmanagertr.h"
#include "cmakegeneratordialogtreemodel.h"

#include <utils/utilsicons.h>
#include <utils/detailswidget.h>

#include <QDialogButtonBox>
#include <QPushButton>
#include <QLayout>
#include <QLabel>

#include <QSplitter>

using namespace Utils;

namespace QmlProjectManager {
namespace GenerateCmake {

CmakeGeneratorDialog::CmakeGeneratorDialog(const FilePath &rootDir,
                                           const FilePaths &files,
                                           const FilePaths invalidFiles)
    : QDialog(), m_rootDir(rootDir), m_files(files), m_invalidFiles(invalidFiles)
{
    setWindowTitle(Tr::tr("Select Files to Generate"));

    QLabel *mainLabel = new QLabel(Tr::tr("Start CMakeFiles.txt generation"), this);
    mainLabel->setMargin(30);

    QVBoxLayout *dialogLayout = new QVBoxLayout(this);
    dialogLayout->addWidget(mainLabel);
    dialogLayout->addWidget(createDetailsWidget());
    dialogLayout->addWidget(createButtons());
    setLayout(dialogLayout);

    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    setMaximumHeight(layout()->totalSizeHint().height());

    refreshNotificationText();
}

QTreeView* CmakeGeneratorDialog::createFileTree()
{
    m_model = new CMakeGeneratorDialogTreeModel(m_rootDir, m_files, this);

    QTreeView *tree = new QTreeView(this);
    tree->setModel(m_model);
    tree->expandAll();
    tree->setHeaderHidden(true);

    return tree;
}

QWidget* CmakeGeneratorDialog::createDetailsWidget()
{
    QTreeView* tree = createFileTree();

    m_notifications = new QTextEdit(this);
    m_warningIcon = Utils::Icons::WARNING.pixmap();

    QSplitter *advancedInnerWidget = new QSplitter(this);
    advancedInnerWidget->addWidget(tree);
    advancedInnerWidget->addWidget(m_notifications);
    advancedInnerWidget->setStretchFactor(0, 2);
    advancedInnerWidget->setStretchFactor(1, 1);
    advancedInnerWidget->setOrientation(Qt::Vertical);
    advancedInnerWidget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::MinimumExpanding);

    DetailsWidget *advancedWidget = new DetailsWidget(this);
    advancedWidget->setMinimumWidth(600);
    advancedWidget->setWidget(advancedInnerWidget);
    advancedWidget->setSummaryText(Tr::tr("Advanced Options"));
    connect(advancedWidget, &DetailsWidget::expanded, this, &CmakeGeneratorDialog::advancedVisibilityChanged);

    return advancedWidget;
}

QWidget* CmakeGeneratorDialog::createButtons()
{
    QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);
    auto *okButton = buttons->button(QDialogButtonBox::Ok);
    okButton->setDefault(true);

    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(m_model, &CMakeGeneratorDialogTreeModel::checkedStateChanged, this, &CmakeGeneratorDialog::refreshNotificationText);

    return buttons;
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

const QString FILE_CREATE_NOTIFICATION = Tr::tr("File %1 will be created.\n");
const QString FILE_OVERWRITE_NOTIFICATION = Tr::tr("File %1 will be overwritten.\n");
const QString FILE_INVALID_NOTIFICATION = Tr::tr(
    "File %1 contains invalid characters and will be skipped.\n");

void CmakeGeneratorDialog::refreshNotificationText()
{
    QTextDocument *document = m_notifications->document();
    document->clear();
    document->addResource(QTextDocument::ImageResource, QUrl("cmakegendialog://warningicon"), m_warningIcon);

    QTextCursor cursor = m_notifications->textCursor();
    QTextImageFormat iformat;
    iformat.setName("cmakegendialog://warningicon");

    QList<CheckableFileTreeItem*> nodes = m_model->items();

    for (const auto &file : m_invalidFiles) {
        cursor.insertImage(iformat);
        cursor.insertText(QString(FILE_INVALID_NOTIFICATION).arg(file.displayName()));
    }

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
            QString relativePath = node->toFilePath().relativePathFrom(m_rootDir).toString();
            cursor.insertImage(iformat);
            cursor.insertText(QString(FILE_OVERWRITE_NOTIFICATION).arg(relativePath));
        }
    }
}

void CmakeGeneratorDialog::advancedVisibilityChanged(bool visible)
{
    if (visible) {
        setMaximumHeight(QWIDGETSIZE_MAX);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    }
    else {
        setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        int height = layout()->totalSizeHint().height();
        setMaximumHeight(height);
        resize(width(), height);
    }
}

} //GenerateCmake
} //QmlProjectManager
