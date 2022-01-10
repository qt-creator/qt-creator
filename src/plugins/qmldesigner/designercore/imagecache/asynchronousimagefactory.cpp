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

#include "asynchronousimagefactory.h"

#include "imagecachegenerator.h"
#include "imagecachestorage.h"
#include "timestampprovider.h"

namespace QmlDesigner {

AsynchronousImageFactory::AsynchronousImageFactory(ImageCacheStorageInterface &storage,
                                                   ImageCacheGeneratorInterface &generator,
                                                   TimeStampProviderInterface &timeStampProvider)
    : m_storage(storage)
    , m_generator(generator)
    , m_timeStampProvider(timeStampProvider)
{
    m_backgroundThread = std::thread{[this] {
        while (isRunning()) {
            if (auto entry = getEntry(); entry) {
                request(entry->name,
                        entry->extraId,
                        std::move(entry->auxiliaryData),
                        m_storage,
                        m_generator,
                        m_timeStampProvider);
            }

            waitForEntries();
        }
    }};
}

AsynchronousImageFactory::~AsynchronousImageFactory()
{
    clean();
    wait();
}

void AsynchronousImageFactory::generate(Utils::PathString name,
                                        Utils::SmallString extraId,
                                        ImageCache::AuxiliaryData auxiliaryData)
{
    addEntry(std::move(name), std::move(extraId), std::move(auxiliaryData));
    m_condition.notify_all();
}

void AsynchronousImageFactory::addEntry(Utils::PathString &&name,
                                        Utils::SmallString &&extraId,
                                        ImageCache::AuxiliaryData &&auxiliaryData)
{
    std::unique_lock lock{m_mutex};

    m_entries.emplace_back(std::move(name), std::move(extraId), std::move(auxiliaryData));
}

bool AsynchronousImageFactory::isRunning()
{
    std::unique_lock lock{m_mutex};
    return !m_finishing || m_entries.size();
}

void AsynchronousImageFactory::waitForEntries()
{
    std::unique_lock lock{m_mutex};
    if (m_entries.empty())
        m_condition.wait(lock, [&] { return m_entries.size() || m_finishing; });
}

std::optional<AsynchronousImageFactory::Entry> AsynchronousImageFactory::getEntry()
{
    std::unique_lock lock{m_mutex};

    if (m_entries.empty())
        return {};

    Entry entry = m_entries.front();
    m_entries.pop_front();

    return {entry};
}

void AsynchronousImageFactory::request(Utils::SmallStringView name,
                                       Utils::SmallStringView extraId,
                                       ImageCache::AuxiliaryData auxiliaryData,
                                       ImageCacheStorageInterface &storage,
                                       ImageCacheGeneratorInterface &generator,
                                       TimeStampProviderInterface &timeStampProvider)
{
    const auto id = extraId.empty() ? Utils::PathString{name}
                                    : Utils::PathString::join({name, "+", extraId});

    const auto currentModifiedTime = timeStampProvider.timeStamp(name);
    const auto storageModifiedTime = storage.fetchModifiedImageTime(id);

    if (currentModifiedTime == storageModifiedTime && storage.fetchHasImage(id))
        return;

    generator.generateImage(name,
                            extraId,
                            currentModifiedTime,
                            ImageCache::CaptureImageWithSmallImageCallback{},
                            ImageCache::AbortCallback{},
                            std::move(auxiliaryData));
}

void AsynchronousImageFactory::clean()
{
    clearEntries();
    m_generator.clean();
}

void AsynchronousImageFactory::wait()
{
    stopThread();
    m_condition.notify_all();
    if (m_backgroundThread.joinable())
        m_backgroundThread.join();
}

void AsynchronousImageFactory::clearEntries()
{
    std::unique_lock lock{m_mutex};
    m_entries.clear();
}

void AsynchronousImageFactory::stopThread()
{
    std::unique_lock lock{m_mutex};
    m_finishing = true;
}

} // namespace QmlDesigner
