// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "imagescaling.h"
#include "downloaddialog.h"

#include <QNetworkReply>

Images::Images(QWidget *parent) : QWidget(parent), downloadDialog(new DownloadDialog(this))
{
    resize(800, 600);

    addUrlsButton = new QPushButton(tr("Add URLs"));
    connect(addUrlsButton, &QPushButton::clicked, this, &Images::process);

    cancelButton = new QPushButton(tr("Cancel"));
    cancelButton->setEnabled(false);
    connect(cancelButton, &QPushButton::clicked, this, &Images::cancel);

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

    connect(&scalingWatcher, &QFutureWatcher<QList<QImage>>::finished,
            this, &Images::scaleFinished);
}

Images::~Images()
{
    cancel();
}

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

        downloadFuture
                .then([this](auto) {
                    cancelButton->setEnabled(false);
                    updateStatus(tr("Scaling..."));
                    scalingWatcher.setFuture(QtConcurrent::run(Images::scaled,
                                                               downloadFuture.results()));
                })
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
                .then([this]() {
                    cancelButton->setEnabled(false);
                    addUrlsButton->setEnabled(true);
                });
    }
}

void Images::cancel()
{
    statusBar->showMessage(tr("Canceling..."));

    downloadFuture.cancel();
    abortDownload();
}

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

QFuture<QByteArray> Images::download(const QList<QUrl> &urls)
{
    QSharedPointer<QPromise<QByteArray>> promise(new QPromise<QByteArray>());
    promise->start();

    for (const auto &url : urls) {
        QSharedPointer<QNetworkReply> reply(qnam.get(QNetworkRequest(url)));
        replies.push_back(reply);

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
            promise->addResult(reply->readAll());

            // Report finished on the last download
            if (promise->future().resultCount() == urls.size())
                promise->finish();
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

    return promise->future();
}

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
