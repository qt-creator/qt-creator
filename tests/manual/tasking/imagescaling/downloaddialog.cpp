// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "downloaddialog.h"
#include "ui_downloaddialog.h"

#include <QUrl>

DownloadDialog::DownloadDialog(QWidget *parent) : QDialog(parent), ui(new Ui::DownloadDialog)
{
    ui->setupUi(this);

    ui->urlLineEdit->setPlaceholderText(tr("Enter the URL of an image to download"));

    connect(ui->addUrlButton, &QPushButton::clicked, this, [this] {
        const auto text = ui->urlLineEdit->text();
        if (!text.isEmpty()) {
            ui->urlListWidget->addItem(text);
            ui->urlLineEdit->clear();
        }
    });
    connect(ui->urlListWidget, &QListWidget::itemSelectionChanged, this, [this] {
        ui->removeUrlButton->setEnabled(!ui->urlListWidget->selectedItems().empty());
    });
    connect(ui->clearUrlsButton, &QPushButton::clicked, ui->urlListWidget, &QListWidget::clear);
    connect(ui->removeUrlButton, &QPushButton::clicked, this,
            [this] { qDeleteAll(ui->urlListWidget->selectedItems()); });
}

DownloadDialog::~DownloadDialog()
{
    delete ui;
}

QList<QUrl> DownloadDialog::getUrls() const
{
    QList<QUrl> urls;
    for (auto row = 0; row < ui->urlListWidget->count(); ++row)
        urls.push_back(QUrl(ui->urlListWidget->item(row)->text()));
    return urls;
}
