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
    stopThread();
    m_condition.notify_all();

    if (m_backgroundThread)
        m_backgroundThread->wait();
}

void ImageCacheGenerator::generateImage(Utils::SmallStringView name,
                                        Sqlite::TimeStamp timeStamp,
                                        ImageCacheGeneratorInterface::CaptureCallback &&captureCallback,
                                        AbortCallback &&abortCallback)
{
    {
        std::lock_guard lock{m_mutex};
        m_tasks.emplace_back(name, timeStamp, std::move(captureCallback), std::move(abortCallback));
    }

    m_condition.notify_all();
}

void ImageCacheGenerator::clean()
{
    std::lock_guard lock{m_mutex};
    for (Task &task : m_tasks)
        task.abortCallback();
    m_tasks.clear();
}

void ImageCacheGenerator::startGeneration()
{
    while (isRunning()) {
        waitForEntries();

        Task task;

        {
            std::lock_guard lock{m_mutex};

            if (m_finishing) {
                m_storage.walCheckpointFull();
                return;
            }

            task = std::move(m_tasks.back());

            m_tasks.pop_back();
        }

        m_collector.start(
            task.filePath,
            [this, task](QImage &&image) {
                if (image.isNull())
                    task.abortCallback();
                else
                    task.captureCallback(image);

                m_storage.storeImage(std::move(task.filePath), task.timeStamp, image);
            },
            [this, task] {
                task.abortCallback();
                m_storage.storeImage(std::move(task.filePath), task.timeStamp, {});
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
    return !m_finishing;
}

} // namespace QmlDesigner
