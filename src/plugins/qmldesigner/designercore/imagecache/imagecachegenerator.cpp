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
{
    m_backgroundThread.reset(QThread::create([this]() { startGeneration(); }));
    m_backgroundThread->start();
}

ImageCacheGenerator::~ImageCacheGenerator()
{
    clean();
    waitForFinished();
}

void ImageCacheGenerator::generateImage(Utils::SmallStringView name,
                                        Utils::SmallStringView extraId,
                                        Sqlite::TimeStamp timeStamp,
                                        ImageCache::CaptureImageWithSmallImageCallback &&captureCallback,
                                        ImageCache::AbortCallback &&abortCallback,
                                        ImageCache::AuxiliaryData &&auxiliaryData)
{
    {
        std::lock_guard lock{m_mutex};

        auto found = std::find_if(m_tasks.begin(), m_tasks.end(), [&](const Task &task) {
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
                                 std::move(abortCallback));
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
    std::lock_guard lock{m_mutex};
    for (Task &task : m_tasks)
        callCallbacks(task.abortCallbacks, ImageCache::AbortReason::Abort);
    m_tasks.clear();
}

void ImageCacheGenerator::waitForFinished()
{
    stopThread();
    m_condition.notify_all();

    if (m_backgroundThread)
        m_backgroundThread->wait();
}

void ImageCacheGenerator::startGeneration()
{
    while (isRunning()) {
        waitForEntries();

        Task task;

        {
            std::lock_guard lock{m_mutex};

            if (m_finishing && m_tasks.empty()) {
                m_storage.walCheckpointFull();
                return;
            }

            task = std::move(m_tasks.front());

            m_tasks.pop_front();
        }

        m_collector.start(
            task.filePath,
            task.extraId,
            std::move(task.auxiliaryData),
            [this, task](const QImage &image, const QImage &smallImage) {
                if (image.isNull())
                    callCallbacks(task.abortCallbacks, ImageCache::AbortReason::Failed);
                else
                    callCallbacks(task.captureCallbacks, image, smallImage);

                m_storage.storeImage(createId(task.filePath, task.extraId),
                                     task.timeStamp,
                                     image,
                                     smallImage);
            },
            [this, task](ImageCache::AbortReason abortReason) {
                callCallbacks(task.abortCallbacks, abortReason);
                if (abortReason != ImageCache::AbortReason::Abort)
                    m_storage.storeImage(createId(task.filePath, task.extraId), task.timeStamp, {}, {});
            });

        std::lock_guard lock{m_mutex};
        if (m_tasks.empty())
            m_storage.walCheckpointFull();
    }
}

void ImageCacheGenerator::waitForEntries()
{
    std::unique_lock lock{m_mutex};
    if (m_tasks.empty())
        m_condition.wait(lock, [&] { return m_tasks.size() || m_finishing; });
}

void ImageCacheGenerator::stopThread()
{
    std::unique_lock lock{m_mutex};
    m_finishing = true;
}

bool ImageCacheGenerator::isRunning()
{
    std::unique_lock lock{m_mutex};
    return !m_finishing || m_tasks.size();
}

} // namespace QmlDesigner
