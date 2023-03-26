// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "addimagesdialog.h"

#include <coreplugin/icore.h>

#include <QComboBox>
#include <QCoreApplication>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QGridLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>

static QTableWidget* createFilesTable(const QStringList &fileNames)
{
    auto table = new QTableWidget(0, 2);
    table->setSelectionMode(QAbstractItemView::NoSelection);

    QStringList labels({
                           QCoreApplication::translate("AddImageToResources","File Name"),
                           QCoreApplication::translate("AddImageToResources","Size")
                       });
    table->setHorizontalHeaderLabels(labels);
    table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    table->verticalHeader()->hide();
    table->setShowGrid(false);

    for (const QString &filePath : fileNames) {
           const QString toolTip = QDir::toNativeSeparators(filePath);
           const QString fileName = QFileInfo(filePath).fileName();
           const qint64 size = QFileInfo(filePath).size() / 1024;
           auto fileNameItem = new QTableWidgetItem(fileName);
           fileNameItem->setToolTip(toolTip);
           fileNameItem->setFlags(fileNameItem->flags() ^ Qt::ItemIsEditable);
           QTableWidgetItem *sizeItem = new QTableWidgetItem(QString::number(size) + " KB");
           sizeItem->setToolTip(toolTip);
           sizeItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
           sizeItem->setFlags(sizeItem->flags() ^ Qt::ItemIsEditable);

           int row = table->rowCount();
           table->insertRow(row);
           table->setItem(row, 0, fileNameItem);
           table->setItem(row, 1, sizeItem);
       }

    return table;
}

QComboBox *createDirectoryComboBox(const QString &defaultDirectory)
{
    auto comboBox = new QComboBox;
    comboBox->addItem(defaultDirectory);
    comboBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    for (const QString &dir :  QDir(defaultDirectory).entryList(QDir::AllDirs | QDir::NoDotAndDotDot))
        comboBox->addItem(defaultDirectory + "/" +dir);

    return comboBox;
}

namespace QmlDesigner {

QString AddImagesDialog::getDirectory(const QStringList &fileNames, const QString &defaultDirectory)
{
    QDialog *dialog = new QDialog(Core::ICore::dialogParent());
    dialog->setMinimumWidth(480);

    QString result;
    QString directory = defaultDirectory;

    dialog->setModal(true);
    dialog->setWindowState(Qt::WindowActive);
    dialog->setWindowTitle(QCoreApplication::translate("AddImageToResources","Add Resources"));
    QTableWidget *table = createFilesTable(fileNames);
    table->setParent(dialog);
    auto mainLayout = new QGridLayout(dialog);
    mainLayout->addWidget(table, 0, 0, 1, 4);

    QComboBox *directoryComboBox = createDirectoryComboBox(defaultDirectory);

    auto setDirectoryForComboBox = [directoryComboBox, &directory](const QString &newDir) {
        if (directoryComboBox->findText(newDir) < 0)
            directoryComboBox->addItem(newDir);

        directoryComboBox->setCurrentText(newDir);
        directory = newDir;
    };

    QObject::connect(directoryComboBox, &QComboBox::currentTextChanged, dialog, [&directory](const QString &text){
       directory = text;
    });

    QPushButton *browseButton = new QPushButton(QCoreApplication::translate("AddImageToResources", "&Browse..."), dialog);

    QObject::connect(browseButton, &QPushButton::clicked, dialog, [setDirectoryForComboBox, &directory]() {
        const QString newDir = QFileDialog::getExistingDirectory(Core::ICore::dialogParent(),
                                                              QCoreApplication::translate("AddImageToResources", "Target Directory"),
                                                              directory);
        if (!newDir.isEmpty())
            setDirectoryForComboBox(newDir);
    });

    mainLayout->addWidget(directoryComboBox, 1, 0, 1, 3);
    mainLayout->addWidget(browseButton, 1, 3, 1 , 1);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok
                                       | QDialogButtonBox::Cancel);

    mainLayout->addWidget(buttonBox, 3, 2, 1, 2);

    QObject::connect(buttonBox, &QDialogButtonBox::accepted, dialog, [dialog](){
        dialog->accept();
        dialog->deleteLater();
    });

    QObject::connect(buttonBox, &QDialogButtonBox::rejected, dialog, [dialog, &directory](){
        dialog->reject();
        dialog->deleteLater();
        directory = QString();
    });

    QObject::connect(dialog, &QDialog::accepted, [&directory, &result](){
        result = directory;
    });

    dialog->exec();

    return result;
}

} //QmlDesigner
