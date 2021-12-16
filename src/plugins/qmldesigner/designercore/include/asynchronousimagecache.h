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

#include "asynchronousimagecacheinterface.h"

#include <condition_variable>
#include <deque>
#include <functional>
#include <mutex>
#include <thread>

namespace QmlDesigner {

class TimeStampProviderInterface;
class ImageCacheStorageInterface;
class ImageCacheGeneratorInterface;
class ImageCacheCollectorInterface;

class AsynchronousImageCache final : public AsynchronousImageCacheInterface
{
public:

    ~AsynchronousImageCache();

    AsynchronousImageCache(ImageCacheStorageInterface &storage,
                           ImageCacheGeneratorInterface &generator,
                           TimeStampProviderInterface &timeStampProvider);

    void requestImage(Utils::PathString name,
                      ImageCache::CaptureImageCallback captureCallback,
                      ImageCache::AbortCallback abortCallback,
                      Utils::SmallString extraId = {},
                      ImageCache::AuxiliaryData auxiliaryData = {}) override;
    void requestSmallImage(Utils::PathString name,
                           ImageCache::CaptureImageCallback captureCallback,
                           ImageCache::AbortCallback abortCallback,
                           Utils::SmallString extraId = {},
                           ImageCache::AuxiliaryData auxiliaryData = {}) override;

    void clean();
    //   void waitForFinished();

private:
    enum class RequestType { Image, SmallImage, Icon };
    struct Entry
    {
        Entry() = default;
        Entry(Utils::PathString name,
              Utils::SmallString extraId,
              ImageCache::CaptureImageCallback &&captureCallback,
              ImageCache::AbortCallback &&abortCallback,
              ImageCache::AuxiliaryData &&auxiliaryData,
              RequestType requestType)
            : name{std::move(name)}
            , extraId{std::move(extraId)}
            , captureCallback{std::move(captureCallback)}
            , abortCallback{std::move(abortCallback)}
            , auxiliaryData{std::move(auxiliaryData)}
            , requestType{requestType}
        {}

        Utils::PathString name;
        Utils::SmallString extraId;
        ImageCache::CaptureImageCallback captureCallback;
        ImageCache::AbortCallback abortCallback;
        ImageCache::AuxiliaryData auxiliaryData;
        RequestType requestType = RequestType::Image;
    };

    std::tuple<bool, Entry> getEntry();
    void addEntry(Utils::PathString &&name,
                  Utils::SmallString &&extraId,
                  ImageCache::CaptureImageCallback &&captureCallback,
                  ImageCache::AbortCallback &&abortCallback,
                  ImageCache::AuxiliaryData &&auxiliaryData,
                  RequestType requestType);
    void clearEntries();
    void waitForEntries();
    void stopThread();
    bool isRunning();
    static void request(Utils::SmallStringView name,
                        Utils::SmallStringView extraId,
                        AsynchronousImageCache::RequestType requestType,
                        ImageCache::CaptureImageCallback captureCallback,
                        ImageCache::AbortCallback abortCallback,
                        ImageCache::AuxiliaryData auxiliaryData,
                        ImageCacheStorageInterface &storage,
                        ImageCacheGeneratorInterface &generator,
                        TimeStampProviderInterface &timeStampProvider);

private:
    void wait();

private:
    std::deque<Entry> m_entries;
    mutable std::mutex m_mutex;
    std::condition_variable m_condition;
    std::thread m_backgroundThread;
    ImageCacheStorageInterface &m_storage;
    ImageCacheGeneratorInterface &m_generator;
    TimeStampProviderInterface &m_timeStampProvider;
    bool m_finishing{false};
};

} // namespace QmlDesigner
