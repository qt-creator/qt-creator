// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "recipe.h"

#include <tasking/concurrentcall.h>
#include <tasking/networkquery.h>

using namespace Tasking;

static void readImage(QPromise<QImage> &promise, const QByteArray &data)
{
    const auto image = QImage::fromData(data);
    if (image.isNull())
        promise.future().cancel();
    else
        promise.addResult(image);
}

static void scaleImage(QPromise<QImage> &promise, const QImage &inputImage, const QSize &size)
{
    promise.addResult(inputImage.scaled(size, Qt::KeepAspectRatio));
}

class InternalData
{
public:
    QByteArray dataSource;
    QImage imageSource;
};

static int sizeForIndex(int index) { return (index + 1) * s_sizeInterval; }

Group recipe(const Storage<ExternalData> &externalStorage)
{
    Storage<InternalData> internalStorage;
    LoopRepeat repeater(s_imageCount);

    const auto onDownloadSetup = [externalStorage](NetworkQuery &query) {
        query.setNetworkAccessManager(externalStorage->inputNam);
        query.setRequest(QNetworkRequest(externalStorage->inputUrl));
    };
    const auto onDownloadDone = [internalStorage, externalStorage](const NetworkQuery &query,
                                                                   DoneWith doneWith) {
        if (doneWith == DoneWith::Success) {
            internalStorage->dataSource = query.reply()->readAll();
        } else {
            externalStorage->outputError
                = QString("Download Error. Code: %1.").arg(query.reply()->error());
        }
    };

    const auto onReadSetup = [internalStorage](ConcurrentCall<QImage> &data) {
        data.setConcurrentCallData(&readImage, internalStorage->dataSource);
    };
    const auto onReadDone = [internalStorage, externalStorage](const ConcurrentCall<QImage> &data,
                                                               DoneWith doneWith) {
        if (doneWith == DoneWith::Success)
            internalStorage->imageSource = data.result();
        else
            externalStorage->outputError = "Image Data Error.";
    };

    const auto onScaleSetup = [internalStorage, repeater](ConcurrentCall<QImage> &data) {
        const int s = sizeForIndex(repeater.iteration());
        data.setConcurrentCallData(&scaleImage, internalStorage->imageSource, QSize(s, s));
    };
    const auto onScaleDone = [externalStorage, repeater](const ConcurrentCall<QImage> &data) {
        externalStorage->outputImages.insert(repeater.iteration(), data.result());
    };

    return Group {
        externalStorage,
        internalStorage,
        NetworkQueryTask(onDownloadSetup, onDownloadDone),
        ConcurrentCallTask<QImage>(onReadSetup, onReadDone),
        For {
            repeater,
            parallelIdealThreadCountLimit,
            ConcurrentCallTask<QImage>(onScaleSetup, onScaleDone)
        }
    };
}
