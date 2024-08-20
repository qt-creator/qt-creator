// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "imagecachegenerator.h"

#include "imagecachecollectorinterface.h"
#include "imagecachestorage.h"

#include <QThread>

namespace QmlDesigner {

ImageCacheGenerator::ImageCacheGenerator(ImageCacheCollectorInterface &collector,
                                         ImageCacheStorageInterface &storage)
    : m_collector{collector}
    , m_storage(storage)
{}

ImageCacheGenerator::~ImageCacheGenerator()
{
    clean();
    waitForFinished();
}

void ImageCacheGenerator::ensureThreadIsRunning()
{
    if (m_finishing)
        return;

    if (m_sleeping) {
        if (m_backgroundThread)
            m_backgroundThread->wait();

        m_sleeping = false;

        m_backgroundThread.reset(QThread::create([this]() { startGeneration(); }));
        m_backgroundThread->start();
    }
}

void ImageCacheGenerator::generateImage(Utils::SmallStringView name,
                                        Utils::SmallStringView extraId,
                                        Sqlite::TimeStamp timeStamp,
                                        ImageCache::CaptureImageWithScaledImagesCallback &&captureCallback,
                                        ImageCache::InternalAbortCallback &&abortCallback,
                                        ImageCache::AuxiliaryData &&auxiliaryData,
                                        ImageCache::TraceToken traceToken)
{
    {
        std::lock_guard lock{m_mutex};

        ensureThreadIsRunning();

        auto found = std::ranges::find_if(m_tasks, [&](const Task &task) {
            return task.filePath == name && task.extraId == extraId;
        });

        if (found != m_tasks.end()) {
            found->timeStamp = timeStamp;
            found->captureCallbacks.push_back(std::move(captureCallback));
            found->abortCallbacks.push_back(std::move(abortCallback));
        } else {
            m_tasks.emplace_back(name,
                                 extraId,
                                 std::move(auxiliaryData),
                                 timeStamp,
                                 std::move(captureCallback),
                                 std::move(abortCallback),
                                 std::move(traceToken));
        }
    }

    m_condition.notify_all();
}

namespace {
Utils::PathString createId(Utils::SmallStringView name, Utils::SmallStringView extraId)
{
    return extraId.empty() ? Utils::PathString{name} : Utils::PathString::join({name, "+", extraId});
}
template<typename Callbacks, typename... Argument>
void callCallbacks(const Callbacks &callbacks, Argument &&...arguments)
{
    for (auto &&callback : callbacks) {
        if (callback)
            callback(std::forward<Argument>(arguments)...);
    }
}

} // namespace

void ImageCacheGenerator::clean()
{
    using namespace NanotraceHR::Literals;

    std::lock_guard lock{m_mutex};
    for (Task &task : m_tasks) {
        callCallbacks(task.abortCallbacks, ImageCache::AbortReason::Abort, std::move(task.traceToken));
    }
    m_tasks.clear();
}

void ImageCacheGenerator::waitForFinished()
{
    stopThread();
    m_condition.notify_all();

    if (m_backgroundThread)
        m_backgroundThread->wait();
}

std::optional<ImageCacheGenerator::Task> ImageCacheGenerator::getTask()
{
    {
        auto [lock, abort] = waitForEntries();

        if (abort)
            return {};

        std::optional<Task> task = std::move(m_tasks.front());

        m_tasks.pop_front();

        return task;
    }
}

void ImageCacheGenerator::startGeneration()
{
    while (true) {
        auto task = getTask();

        if (!task)
            return;

        m_collector.start(
            task->filePath,
            task->extraId,
            std::move(task->auxiliaryData),
            [this,
             abortCallbacks = task->abortCallbacks,
             captureCallbacks = std::move(task->captureCallbacks),
             filePath = task->filePath,
             extraId = task->extraId,
             timeStamp = task->timeStamp](const QImage &image,
                                          const QImage &midSizeImage,
                                          const QImage &smallImage,
                                          ImageCache::TraceToken traceToken) {
                if (image.isNull() && midSizeImage.isNull() && smallImage.isNull())
                    callCallbacks(abortCallbacks,
                                  ImageCache::AbortReason::Failed,
                                  std::move(traceToken));
                else
                    callCallbacks(captureCallbacks,
                                  image,
                                  midSizeImage,
                                  smallImage,
                                  std::move(traceToken));

                m_storage.storeImage(createId(filePath, extraId),
                                     timeStamp,
                                     image,
                                     midSizeImage,
                                     smallImage);
            },
            [this,
             abortCallbacks = task->abortCallbacks,
             filePath = task->filePath,
             extraId = task->extraId,
             timeStamp = task->timeStamp](ImageCache::AbortReason abortReason,
                                          ImageCache::TraceToken traceToken) {
                callCallbacks(abortCallbacks, abortReason, std::move(traceToken));
                if (abortReason != ImageCache::AbortReason::Abort)
                    m_storage.storeImage(createId(filePath, extraId), timeStamp, {}, {}, {});
            },
            std::move(task->traceToken));

        std::lock_guard lock{m_mutex};
        if (m_tasks.empty())
            m_storage.walCheckpointFull();
    }
}

std::tuple<std::unique_lock<std::mutex>, bool> ImageCacheGenerator::waitForEntries()
{
    using namespace std::literals::chrono_literals;
    std::unique_lock lock{m_mutex};
    if (m_finishing)
        return {std::move(lock), true};
    if (m_tasks.empty()) {
        auto timedOutWithoutEntriesOrFinishing = !m_condition.wait_for(lock, 10min, [&] {
            return m_tasks.size() || m_finishing;
        });

        if (timedOutWithoutEntriesOrFinishing || m_finishing) {
            m_sleeping = true;
            return {std::move(lock), true};
        }
    }
    return {std::move(lock), false};
}

void ImageCacheGenerator::stopThread()
{
    std::unique_lock lock{m_mutex};
    m_finishing = true;
}

} // namespace QmlDesigner
