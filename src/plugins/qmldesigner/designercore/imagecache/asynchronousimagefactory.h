/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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

#include "imagecacheauxiliarydata.h"

#include <utils/optional.h>
#include <utils/smallstring.h>

#include <condition_variable>
#include <deque>
#include <mutex>
#include <thread>

namespace QmlDesigner {

class TimeStampProviderInterface;
class ImageCacheStorageInterface;
class ImageCacheGeneratorInterface;
class ImageCacheCollectorInterface;

class AsynchronousImageFactory
{
public:
    AsynchronousImageFactory(ImageCacheStorageInterface &storage,
                             ImageCacheGeneratorInterface &generator,
                             TimeStampProviderInterface &timeStampProvider);

    ~AsynchronousImageFactory();

    void generate(Utils::PathString name,
                  Utils::SmallString extraId = {},
                  ImageCache::AuxiliaryData auxiliaryData = {});
    void clean();

private:
    struct Entry
    {
        Entry() = default;
        Entry(Utils::PathString name,
              Utils::SmallString extraId,
              ImageCache::AuxiliaryData &&auxiliaryData)
            : name{std::move(name)}
            , extraId{std::move(extraId)}
            , auxiliaryData{std::move(auxiliaryData)}
        {}

        Utils::PathString name;
        Utils::SmallString extraId;
        ImageCache::AuxiliaryData auxiliaryData;
    };

    void addEntry(Utils::PathString &&name,
                  Utils::SmallString &&extraId,
                  ImageCache::AuxiliaryData &&auxiliaryData);
    bool isRunning();
    void waitForEntries();
    Utils::optional<Entry> getEntry();
    void request(Utils::SmallStringView name,
                 Utils::SmallStringView extraId,
                 ImageCache::AuxiliaryData auxiliaryData,
                 ImageCacheStorageInterface &storage,
                 ImageCacheGeneratorInterface &generator,
                 TimeStampProviderInterface &timeStampProvider);
    void wait();
    void clearEntries();
    void stopThread();

private:
    std::deque<Entry> m_entries;
    std::mutex m_mutex;
    std::condition_variable m_condition;
    std::thread m_backgroundThread;
    ImageCacheStorageInterface &m_storage;
    ImageCacheGeneratorInterface &m_generator;
    TimeStampProviderInterface &m_timeStampProvider;
    bool m_finishing{false};
};

} // namespace QmlDesigner
