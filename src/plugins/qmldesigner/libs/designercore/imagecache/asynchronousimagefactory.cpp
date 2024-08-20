// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "asynchronousimagefactory.h"

#include "imagecachecollectorinterface.h"
#include "imagecachegenerator.h"
#include "imagecachestorage.h"
#include "timestampproviderinterface.h"

namespace QmlDesigner {

using namespace NanotraceHR::Literals;

AsynchronousImageFactory::AsynchronousImageFactory(ImageCacheStorageInterface &storage,
                                                   TimeStampProviderInterface &timeStampProvider,
                                                   ImageCacheCollectorInterface &collector)
    : m_storage(storage)
    , m_timeStampProvider(timeStampProvider)
    , m_collector(collector)
{

}

void AsynchronousImageFactory::generate(Utils::SmallStringView name,
                                        Utils::SmallStringView extraId,
                                        ImageCache::AuxiliaryData auxiliaryData)
{
    auto [trace, flowToken] = ImageCache::category().beginDurationWithFlow(
        "request image in asynchronous image factory"_t);
    m_taskQueue.addTask(trace.createToken(),
                        name,
                        extraId,
                        std::move(auxiliaryData),
                        std::move(flowToken));
}

AsynchronousImageFactory::~AsynchronousImageFactory() {}

void AsynchronousImageFactory::request(Utils::SmallStringView name,
                                       Utils::SmallStringView extraId,
                                       ImageCache::AuxiliaryData auxiliaryData,
                                       ImageCacheStorageInterface &storage,
                                       TimeStampProviderInterface &timeStampProvider,
                                       ImageCacheCollectorInterface &collector,
                                       ImageCache::TraceToken traceToken)
{
    auto [storageTracer, flowToken] = traceToken.beginDurationWithFlow("starte image generator"_t);
    const auto id = extraId.empty() ? Utils::PathString{name}
                                    : Utils::PathString::join({name, "+", extraId});

    const auto currentModifiedTime = timeStampProvider.timeStamp(name);
    const auto storageModifiedTime = storage.fetchModifiedImageTime(id);
    const auto pause = timeStampProvider.pause();

    if (currentModifiedTime < (storageModifiedTime + pause))
        return;

    auto capture = [&](const QImage &image,
                       const QImage &midSizeImage,
                       const QImage &smallImage,
                       ImageCache::TraceToken) {
        storage.storeImage(id, currentModifiedTime, image, midSizeImage, smallImage);
    };

    collector.start(name,
                    extraId,
                    std::move(auxiliaryData),
                    std::move(capture),
                    ImageCache::InternalAbortCallback{},
                    std::move(flowToken));
}

void AsynchronousImageFactory::clean()
{
    m_taskQueue.clean();
}

void AsynchronousImageFactory::Dispatch::operator()(Entry &entry)
{
    request(entry.name,
            entry.extraId,
            std::move(entry.auxiliaryData),
            storage,
            timeStampProvider,
            collector,
            std::move(entry.traceToken));
}
} // namespace QmlDesigner
