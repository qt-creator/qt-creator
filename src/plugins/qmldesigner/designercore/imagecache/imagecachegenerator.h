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

#pragma once

#include "imagecachegeneratorinterface.h"

#include <utils/smallstring.h>

#include <QThread>

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
    ImageCacheGenerator(ImageCacheCollectorInterface &collector, ImageCacheStorageInterface &storage)
        : m_collector{collector}
        , m_storage(storage)
    {}

    ~ImageCacheGenerator();

    void generateImage(Utils::SmallStringView filePath,
                       Sqlite::TimeStamp timeStamp,
                       CaptureCallback &&captureCallback,
                       AbortCallback &&abortCallback) override;
    void clean() override;

private:
    struct Task
    {
        Task() = default;
        Task(Utils::SmallStringView filePath,
             Sqlite::TimeStamp timeStamp,
             CaptureCallback &&captureCallback,
             AbortCallback &&abortCallback)
            : filePath(filePath)
            , captureCallback(std::move(captureCallback))
            , abortCallback(std::move(abortCallback))
            , timeStamp(timeStamp)
        {}

        Utils::PathString filePath;
        CaptureCallback captureCallback;
        AbortCallback abortCallback;
        Sqlite::TimeStamp timeStamp;
    };

    void startGeneration(std::shared_ptr<std::mutex> threadMutex);
    void startGenerationAsynchronously();

private:
    std::unique_ptr<QThread> m_backgroundThread;
    std::mutex m_dataMutex;
    std::shared_ptr<std::mutex> m_threadMutex{std::make_shared<std::mutex>()};
    std::vector<Task> m_tasks;
    ImageCacheCollectorInterface &m_collector;
    ImageCacheStorageInterface &m_storage;
    std::atomic_flag m_processing = ATOMIC_FLAG_INIT;
};

} // namespace QmlDesigner
