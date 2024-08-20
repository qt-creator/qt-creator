// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "imagecachegeneratorinterface.h"

#include <imagecacheauxiliarydata.h>
#include <qmldesignercorelib_exports.h>
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

class QMLDESIGNERCORE_EXPORT ImageCacheGenerator final : public ImageCacheGeneratorInterface
{
public:
    ImageCacheGenerator(ImageCacheCollectorInterface &collector, ImageCacheStorageInterface &storage);

    ~ImageCacheGenerator();

    void generateImage(Utils::SmallStringView filePath,
                       Utils::SmallStringView extraId,
                       Sqlite::TimeStamp timeStamp,
                       ImageCache::CaptureImageWithScaledImagesCallback &&captureCallback,
                       ImageCache::InternalAbortCallback &&abortCallback,
                       ImageCache::AuxiliaryData &&auxiliaryData,
                       ImageCache::TraceToken traceToken = {}) override;
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
             ImageCache::InternalAbortCallback &&abortCallback,
             ImageCache::TraceToken traceToken)
            : filePath(filePath)
            , extraId(std::move(extraId))
            , auxiliaryData(std::move(auxiliaryData))
            , captureCallbacks({std::move(captureCallback)})
            , abortCallbacks({std::move(abortCallback)})
            , timeStamp(timeStamp)
            , traceToken{std::move(traceToken)}
        {}

        Utils::PathString filePath;
        Utils::SmallString extraId;
        ImageCache::AuxiliaryData auxiliaryData;
        std::vector<ImageCache::CaptureImageWithScaledImagesCallback> captureCallbacks;
        std::vector<ImageCache::InternalAbortCallback> abortCallbacks;
        Sqlite::TimeStamp timeStamp;
        NO_UNIQUE_ADDRESS ImageCache::TraceToken traceToken;
    };

    void startGeneration();
    std::optional<Task> getTask();
    void ensureThreadIsRunning();
    [[nodiscard]] std::tuple<std::unique_lock<std::mutex>, bool> waitForEntries();
    void stopThread();

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
