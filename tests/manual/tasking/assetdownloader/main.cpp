// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "assetdownloader.h"

#include <QApplication>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QProgressDialog>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setOrganizationName("QtProject");
    app.setApplicationName("Asset Downloader");

    QProgressDialog progress;
    progress.setAutoClose(false);
    progress.setRange(0, 0);
    QObject::connect(&progress, &QProgressDialog::canceled, &app, &QApplication::quit);

    QNetworkAccessManager manager;

    AssetDownloader downloader;
    downloader.setNetworkAccessManager(&manager);
    downloader.setJsonFileName("car-configurator-assets-v1.json");
    downloader.setZipFileName("car-configurator-assets-v1.zip");
    downloader.setDownloadBase(QUrl("https://download.qt.io/learning/examples/"));

    QObject::connect(&downloader, &AssetDownloader::started,
                     &progress, &QProgressDialog::show);
    QObject::connect(&downloader, &AssetDownloader::progressChanged, &progress,
                     [&](int progressValue, int progressMaximum, const QString &progressText) {
        progress.setLabelText(progressText);
        progress.setMaximum(progressMaximum);
        progress.setValue(progressValue);
    });
    QObject::connect(&downloader, &AssetDownloader::finished, &progress, [&](bool success) {
        progress.reset();
        progress.hide();
        if (success)
            QMessageBox::information(nullptr, "Asset Downloader", "Download Finished Successfully.");
        else
            QMessageBox::warning(nullptr, "Asset Downloader", "Download Finished with an Error.");
    });

    downloader.start();

    return app.exec();
}
