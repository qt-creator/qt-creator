// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "imagescaling.h"
#include "downloaddialog.h"

#include <QNetworkReply>

Images::Images(QWidget *parent) : QWidget(parent), downloadDialog(new DownloadDialog(this))
{
    resize(800, 600);

    addUrlsButton = new QPushButton(tr("Add URLs"));
//! [1]
    connect(addUrlsButton, &QPushButton::clicked, this, &Images::process);
//! [1]

    cancelButton = new QPushButton(tr("Cancel"));
    cancelButton->setEnabled(false);
//! [2]
    connect(cancelButton, &QPushButton::clicked, this, &Images::cancel);
//! [2]

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(addUrlsButton);
    buttonLayout->addWidget(cancelButton);
    buttonLayout->addStretch();

    statusBar = new QStatusBar();

    imagesLayout = new QGridLayout();

    mainLayout = new QVBoxLayout();
    mainLayout->addLayout(buttonLayout);
    mainLayout->addLayout(imagesLayout);
    mainLayout->addStretch();
    mainLayout->addWidget(statusBar);
    setLayout(mainLayout);

//! [6]
    connect(&scalingWatcher, &QFutureWatcher<QList<QImage>>::finished,
            this, &Images::scaleFinished);
//! [6]
}

Images::~Images()
{
    cancel();
}

//! [3]
void Images::process()
{
    // Clean previous state
    replies.clear();
    addUrlsButton->setEnabled(false);

    if (downloadDialog->exec() == QDialog::Accepted) {

        const auto urls = downloadDialog->getUrls();
        if (urls.empty())
            return;

        cancelButton->setEnabled(true);

        initLayout(urls.size());

        downloadFuture = download(urls);
        statusBar->showMessage(tr("Downloading..."));
//! [3]

        //! [4]
        downloadFuture
                .then([this](auto) {
                    cancelButton->setEnabled(false);
                    updateStatus(tr("Scaling..."));
                    //! [16]
                    scalingWatcher.setFuture(QtConcurrent::run(Images::scaled,
                                                               downloadFuture.results()));
                    //! [16]
                })
        //! [4]
        //! [5]
                .onCanceled([this] {
                    updateStatus(tr("Download has been canceled."));
                })
                .onFailed([this](QNetworkReply::NetworkError error) {
                    updateStatus(tr("Download finished with error: %1").arg(error));
                    // Abort all pending requests
                    abortDownload();
                })
                .onFailed([this](const std::exception &ex) {
                    updateStatus(tr(ex.what()));
                })
        //! [5]
                .then([this]() {
                    cancelButton->setEnabled(false);
                    addUrlsButton->setEnabled(true);
                });
    }
}

//! [7]
void Images::cancel()
{
    statusBar->showMessage(tr("Canceling..."));

    downloadFuture.cancel();
    abortDownload();
}
//! [7]

//! [15]
void Images::scaleFinished()
{
    const OptionalImages result = scalingWatcher.result();
    if (result.has_value()) {
        const auto scaled = result.value();
        showImages(scaled);
        updateStatus(tr("Finished"));
    } else {
        updateStatus(tr("Failed to extract image data."));
    }
    addUrlsButton->setEnabled(true);
}
//! [15]

//! [8]
QFuture<QByteArray> Images::download(const QList<QUrl> &urls)
{
//! [8]
//! [9]
    QSharedPointer<QPromise<QByteArray>> promise(new QPromise<QByteArray>());
    promise->start();
//! [9]

    //! [10]
    for (const auto &url : urls) {
        QSharedPointer<QNetworkReply> reply(qnam.get(QNetworkRequest(url)));
        replies.push_back(reply);
    //! [10]

    //! [11]
        QtFuture::connect(reply.get(), &QNetworkReply::finished).then([=] {
            if (promise->isCanceled()) {
                if (!promise->future().isFinished())
                    promise->finish();
                return;
            }

            if (reply->error() != QNetworkReply::NoError) {
                if (!promise->future().isFinished())
                    throw reply->error();
            }
        //! [12]
            promise->addResult(reply->readAll());

            // Report finished on the last download
            if (promise->future().resultCount() == urls.size())
                promise->finish();
        //! [12]
        }).onFailed([promise] (QNetworkReply::NetworkError error) {
            promise->setException(std::make_exception_ptr(error));
            promise->finish();
        }).onFailed([promise] {
            const auto ex = std::make_exception_ptr(
                        std::runtime_error("Unknown error occurred while downloading."));
            promise->setException(ex);
            promise->finish();
        });
    }
    //! [11]

//! [13]
    return promise->future();
}
//! [13]

//! [14]
Images::OptionalImages Images::scaled(const QList<QByteArray> &data)
{
    QList<QImage> scaled;
    for (const auto &imgData : data) {
        QImage image;
        image.loadFromData(imgData);
        if (image.isNull())
            return std::nullopt;

        scaled.push_back(image.scaled(100, 100, Qt::KeepAspectRatio));
    }

    return scaled;
}
//! [14]

void Images::showImages(const QList<QImage> &images)
{
    for (int i = 0; i < images.size(); ++i) {
        labels[i]->setAlignment(Qt::AlignCenter);
        labels[i]->setPixmap(QPixmap::fromImage(images[i]));
    }
}

void Images::initLayout(qsizetype count)
{
    // Clean old images
    QLayoutItem *child;
    while ((child = imagesLayout->takeAt(0)) != nullptr) {
        child->widget()->setParent(nullptr);
        delete child->widget();
        delete child;
    }
    labels.clear();

    // Init the images layout for the new images
    const auto dim = int(qSqrt(qreal(count))) + 1;
    for (int i = 0; i < dim; ++i) {
        for (int j = 0; j < dim; ++j) {
            QLabel *imageLabel = new QLabel;
            imageLabel->setFixedSize(100, 100);
            imagesLayout->addWidget(imageLabel, i, j);
            labels.append(imageLabel);
        }
    }
}

void Images::updateStatus(const QString &msg)
{
    statusBar->showMessage(msg);
}

void Images::abortDownload()
{
    for (auto reply : replies)
        reply->abort();
}
