/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "imagecachegenerator.h"

#include "imagecachecollectorinterface.h"
#include "imagecachestorage.h"

#include <QThread>

namespace QmlDesigner {

ImageCacheGenerator::~ImageCacheGenerator()
{
    std::lock_guard threadLock{*m_threadMutex.get()};

    if (m_backgroundThread)
        m_backgroundThread->wait();

    clean();
}

void ImageCacheGenerator::generateImage(Utils::SmallStringView name,
                                        Sqlite::TimeStamp timeStamp,
                                        ImageCacheGeneratorInterface::CaptureCallback &&captureCallback,
                                        AbortCallback &&abortCallback)
{
    {
        std::lock_guard lock{m_dataMutex};
        m_tasks.emplace_back(name, timeStamp, std::move(captureCallback), std::move(abortCallback));
    }

    startGenerationAsynchronously();
}

void ImageCacheGenerator::clean()
{
    std::lock_guard dataLock{m_dataMutex};
    m_tasks.clear();
}

class ReleaseProcessing
{
public:
    ReleaseProcessing(std::atomic_flag &processing)
        : m_processing(processing)
    {
        m_processing.test_and_set(std::memory_order_acquire);
    }

    ~ReleaseProcessing() { m_processing.clear(std::memory_order_release); }

private:
    std::atomic_flag &m_processing;
};

void ImageCacheGenerator::startGeneration(std::shared_ptr<std::mutex> threadMutex)
{
    ReleaseProcessing guard(m_processing);

    while (true) {
        Task task;

        {
            std::unique_lock threadLock{*threadMutex.get(), std::defer_lock_t{}};

            if (!threadLock.try_lock())
                return;

            std::lock_guard dataLock{m_dataMutex};

            if (m_tasks.empty()) {
                m_storage.walCheckpointFull();
                return;
            }

            task = std::move(m_tasks.back());

            m_tasks.pop_back();
        }

        m_collector.start(
            task.filePath,
            [this, threadMutex, task](QImage &&image) {
                std::unique_lock lock{*threadMutex.get(), std::defer_lock_t{}};

                if (!lock.try_lock())
                    return;

                if (threadMutex.use_count() == 1)
                    return;

                if (image.isNull())
                    task.abortCallback();
                else
                    task.captureCallback(image);

                m_storage.storeImage(std::move(task.filePath), task.timeStamp, image);
            },
            [this, threadMutex, task] {
                std::unique_lock lock{*threadMutex.get(), std::defer_lock_t{}};

                if (!lock.try_lock())
                    return;

                if (threadMutex.use_count() == 1)
                    return;

                task.abortCallback();
                m_storage.storeImage(std::move(task.filePath), task.timeStamp, {});
            });
    }
}

void ImageCacheGenerator::startGenerationAsynchronously()
{
    if (m_processing.test_and_set(std::memory_order_acquire))
        return;

    std::unique_lock lock{*m_threadMutex.get(), std::defer_lock_t{}};

    if (!lock.try_lock())
        return;

    if (m_backgroundThread)
        m_backgroundThread->wait();

    m_backgroundThread.reset(QThread::create(
        [this](std::shared_ptr<std::mutex> threadMutex) { startGeneration(threadMutex); },
        m_threadMutex));
    m_backgroundThread->start();
    //    m_backgroundThread = std::thread(
    //        [this](std::shared_ptr<std::mutex> threadMutex) { startGeneration(threadMutex); },
    //        m_threadMutex);
}

} // namespace QmlDesigner
