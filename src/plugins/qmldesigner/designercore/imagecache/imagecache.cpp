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

#include "imagecache.h"

#include "imagecachegenerator.h"
#include "imagecachestorage.h"
#include "timestampprovider.h"

#include <thread>

namespace QmlDesigner {

ImageCache::ImageCache(ImageCacheStorageInterface &storage,
                       ImageCacheGeneratorInterface &generator,
                       TimeStampProviderInterface &timeStampProvider)
    : m_storage(storage)
    , m_generator(generator)
    , m_timeStampProvider(timeStampProvider)
{
    m_backgroundThread = std::thread{[this] {
        while (isRunning()) {
            if (auto [hasEntry, entry] = getEntry(); hasEntry) {
                request(entry.name,
                        entry.requestType,
                        std::move(entry.captureCallback),
                        std::move(entry.abortCallback),
                        m_storage,
                        m_generator,
                        m_timeStampProvider);
            }

            waitForEntries();
        }
    }};
}

ImageCache::~ImageCache()
{
    clean();
    stopThread();
    m_condition.notify_all();
    if (m_backgroundThread.joinable())
        m_backgroundThread.join();
}

void ImageCache::request(Utils::SmallStringView name,
                         ImageCache::RequestType requestType,
                         ImageCache::CaptureCallback captureCallback,
                         ImageCache::AbortCallback abortCallback,
                         ImageCacheStorageInterface &storage,
                         ImageCacheGeneratorInterface &generator,
                         TimeStampProviderInterface &timeStampProvider)
{
    const auto timeStamp = timeStampProvider.timeStamp(name);
    const auto entry = requestType == RequestType::Image ? storage.fetchImage(name, timeStamp)
                                                         : storage.fetchIcon(name, timeStamp);

    if (entry.hasEntry) {
        if (entry.image.isNull())
            abortCallback();
        else
            captureCallback(entry.image);
    } else {
        generator.generateImage(name, timeStamp, std::move(captureCallback), std::move(abortCallback));
    }
}

void ImageCache::requestImage(Utils::PathString name,
                              ImageCache::CaptureCallback captureCallback,
                              AbortCallback abortCallback)
{
    addEntry(std::move(name), std::move(captureCallback), std::move(abortCallback), RequestType::Image);
    m_condition.notify_all();
}

void ImageCache::requestIcon(Utils::PathString name,
                             ImageCache::CaptureCallback captureCallback,
                             ImageCache::AbortCallback abortCallback)
{
    addEntry(std::move(name), std::move(captureCallback), std::move(abortCallback), RequestType::Icon);
    m_condition.notify_all();
}

void ImageCache::clean()
{
    clearEntries();
    m_generator.clean();
}

std::tuple<bool, ImageCache::Entry> ImageCache::getEntry()
{
    std::unique_lock lock{m_mutex};

    if (m_entries.empty())
        return {false, Entry{}};

    Entry entry = m_entries.back();
    m_entries.pop_back();

    return {true, entry};
}

void ImageCache::addEntry(Utils::PathString &&name,
                          ImageCache::CaptureCallback &&captureCallback,
                          AbortCallback &&abortCallback,
                          RequestType requestType)
{
    std::unique_lock lock{m_mutex};

    m_entries.emplace_back(std::move(name),
                           std::move(captureCallback),
                           std::move(abortCallback),
                           requestType);
}

void ImageCache::clearEntries()
{
    std::unique_lock lock{m_mutex};
    for (Entry &entry : m_entries)
        entry.abortCallback();
    m_entries.clear();
}

void ImageCache::waitForEntries()
{
    std::unique_lock lock{m_mutex};
    if (m_entries.empty())
        m_condition.wait(lock, [&] { return m_entries.size() || m_finishing; });
}

void ImageCache::stopThread()
{
    std::unique_lock lock{m_mutex};
    m_finishing = true;
}

bool ImageCache::isRunning()
{
    std::unique_lock lock{m_mutex};
    return !m_finishing;
}

} // namespace QmlDesigner
