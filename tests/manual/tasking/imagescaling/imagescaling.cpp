// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "imagescaling.h"
#include "downloaddialog.h"
#include <tasking/concurrentcall.h>
#include <tasking/networkquery.h>

using namespace Tasking;

Images::Images(QWidget *parent) : QWidget(parent), downloadDialog(new DownloadDialog(this))
{
    resize(800, 600);

    QPushButton *addUrlsButton = new QPushButton(tr("Add URLs"));
    connect(addUrlsButton, &QPushButton::clicked, this, &Images::process);

    cancelButton = new QPushButton(tr("Cancel"));
    cancelButton->setEnabled(false);
    connect(cancelButton, &QPushButton::clicked, this, [this] {
        statusBar->showMessage(tr("Canceled."));
        taskTree.reset();
    });

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
}

static void scale(QPromise<QImage> &promise, const QByteArray &data)
{
    const auto image = QImage::fromData(data);
    if (image.isNull())
        promise.future().cancel();
    else
        promise.addResult(image.scaled(100, 100, Qt::KeepAspectRatio));
}

void Images::process()
{
    if (downloadDialog->exec() != QDialog::Accepted)
        return;

    const auto urls = downloadDialog->getUrls();
    initLayout(urls.size());

    const auto onRootSetup = [this] {
        statusBar->showMessage(tr("Downloading and Scaling..."));
        cancelButton->setEnabled(true);
    };
    const auto onRootDone = [this] {
        statusBar->showMessage(tr("Finished."));
        cancelButton->setEnabled(false);
    };
    QList<GroupItem> tasks {
        finishAllAndDone,
        parallel,
        onGroupSetup(onRootSetup),
        onGroupDone(onRootDone)
    };

    int i = 0;
    for (const QUrl &url : urls) {
        TreeStorage<QByteArray> storage;

        const auto onDownloadSetup = [this, url](NetworkQuery &query) {
            query.setNetworkAccessManager(&qnam);
            query.setRequest(QNetworkRequest(url));
        };
        const auto onDownloadDone = [storage](const NetworkQuery &query) {
            *storage = query.reply()->readAll();
        };
        const auto onDownloadError = [this, i](const NetworkQuery &query) {
            labels[i]->setText(tr("Download\nError.\nCode: %1.").arg(query.reply()->error()));
        };

        const auto onScalingSetup = [storage](ConcurrentCall<QImage> &data) {
            data.setConcurrentCallData(&scale, *storage);
        };
        const auto onScalingDone = [this, i](const ConcurrentCall<QImage> &data) {
            labels[i]->setPixmap(QPixmap::fromImage(data.result()));
        };
        const auto onScalingError = [this, i](const ConcurrentCall<QImage> &) {
            labels[i]->setText(tr("Image\nData\nError."));
        };

        const Group group {
            Storage(storage),
            NetworkQueryTask(onDownloadSetup, onDownloadDone, onDownloadError),
            ConcurrentCallTask<QImage>(onScalingSetup, onScalingDone, onScalingError)
        };
        tasks.append(group);
        ++i;
    }

    taskTree.reset(new TaskTree(tasks));
    connect(taskTree.get(), &TaskTree::done, this, [this] { taskTree.release()->deleteLater(); });
    taskTree->start();
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
            imageLabel->setAlignment(Qt::AlignCenter);
            imagesLayout->addWidget(imageLabel, i, j);
            labels.append(imageLabel);
        }
    }
}
