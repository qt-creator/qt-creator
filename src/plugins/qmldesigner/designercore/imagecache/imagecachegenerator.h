// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "imagecachegeneratorinterface.h"

#include <imagecacheauxiliarydata.h>
#include <utils/smallstring.h>

#include <QThread>

#include <deque>
#include <memory>
#include <mutex>

QT_BEGIN_NAMESPACE
class QPlainTextEdit;
QT_END_NAMESPACE

namespace QmlDesigner {

class ImageCacheCollectorInterface;
class ImageCacheStorageInterface;

class ImageCacheGenerator final : public ImageCacheGeneratorInterface
{
public:
    ImageCacheGenerator(ImageCacheCollectorInterface &collector, ImageCacheStorageInterface &storage);

    ~ImageCacheGenerator();

    void generateImage(Utils::SmallStringView filePath,
                       Utils::SmallStringView extraId,
                       Sqlite::TimeStamp timeStamp,
                       ImageCache::CaptureImageWithScaledImagesCallback &&captureCallback,
                       ImageCache::AbortCallback &&abortCallback,
                       ImageCache::AuxiliaryData &&auxiliaryData) override;
    void clean() override;

    void waitForFinished() override;

private:
    struct Task
    {
        Task() = default;
        Task(Utils::SmallStringView filePath,
             Utils::SmallStringView extraId,
             ImageCache::AuxiliaryData &&auxiliaryData,
             Sqlite::TimeStamp timeStamp,
             ImageCache::CaptureImageWithScaledImagesCallback &&captureCallback,
             ImageCache::AbortCallback &&abortCallback)
            : filePath(filePath)
            , extraId(std::move(extraId))
            , auxiliaryData(std::move(auxiliaryData))
            , captureCallbacks({std::move(captureCallback)})
            , abortCallbacks({std::move(abortCallback)})
            , timeStamp(timeStamp)
        {}

        Utils::PathString filePath;
        Utils::SmallString extraId;
        ImageCache::AuxiliaryData auxiliaryData;
        std::vector<ImageCache::CaptureImageWithScaledImagesCallback> captureCallbacks;
        std::vector<ImageCache::AbortCallback> abortCallbacks;
        Sqlite::TimeStamp timeStamp;
    };

    void startGeneration();
    void ensureThreadIsRunning();
    [[nodiscard]] std::tuple<std::unique_lock<std::mutex>, bool> waitForEntries();
    void stopThread();

private:
private:
    std::unique_ptr<QThread> m_backgroundThread;
    mutable std::mutex m_mutex;
    std::condition_variable m_condition;
    std::deque<Task> m_tasks;
    ImageCacheCollectorInterface &m_collector;
    ImageCacheStorageInterface &m_storage;
    bool m_finishing{false};
    bool m_sleeping{true};
};

} // namespace QmlDesigner
