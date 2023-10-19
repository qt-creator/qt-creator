// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "asynchronousimagecache.h"

#include "imagecachegenerator.h"
#include "imagecachestorage.h"
#include "timestampprovider.h"

#include <tracing/qmldesignertracing.h>

#include <QScopeGuard>

#include <thread>

namespace QmlDesigner {

using namespace NanotraceHR::Literals;

namespace ImageCache {
namespace {

thread_local Category category_{"image cache"_t,
                                QmlDesigner::Tracing::eventQueueWithStringArguments(),
                                category};
} // namespace

Category &category()
{
    return category_;
}
} // namespace ImageCache

AsynchronousImageCache::AsynchronousImageCache(ImageCacheStorageInterface &storage,
                                               ImageCacheGeneratorInterface &generator,
                                               TimeStampProviderInterface &timeStampProvider)
    : m_storage(storage)
    , m_generator(generator)
    , m_timeStampProvider(timeStampProvider)
{}

AsynchronousImageCache::~AsynchronousImageCache()
{
    m_generator.clean();
}

void AsynchronousImageCache::request(Utils::SmallStringView name,
                                     Utils::SmallStringView extraId,
                                     AsynchronousImageCache::RequestType requestType,
                                     ImageCache::CaptureImageCallback captureCallback,
                                     ImageCache::AbortCallback abortCallback,
                                     ImageCache::AuxiliaryData auxiliaryData,
                                     ImageCache::TraceToken traceToken,
                                     ImageCacheStorageInterface &storage,
                                     ImageCacheGeneratorInterface &generator,
                                     TimeStampProviderInterface &timeStampProvider)
{
    const auto id = extraId.empty() ? Utils::PathString{name}
                                    : Utils::PathString::join({name, "+", extraId});

    using NanotraceHR::dictonary;
    using NanotraceHR::keyValue;
    using namespace std::literals::string_view_literals;

    auto [durationToken, flowToken] = traceToken.beginDurationWithFlow(
        "AsynchronousImageCache works on the image request"_t,
        keyValue("name", name),
        keyValue("extra id", extraId));

    auto timeStrampToken = durationToken.beginDuration("getting timestamp"_t);
    const auto timeStamp = timeStampProvider.timeStamp(name);
    timeStrampToken.end(keyValue("time stamp", timeStamp.value));

    auto storageTraceToken = durationToken.beginDuration("fetching image from storage"_t,
                                                         keyValue("storage id", id));
    auto requestImageFromStorage = [&](RequestType requestType) {
        switch (requestType) {
        case RequestType::Image:
            return storage.fetchImage(id, timeStamp);
        case RequestType::MidSizeImage:
            return storage.fetchMidSizeImage(id, timeStamp);
        case RequestType::SmallImage:
            return storage.fetchSmallImage(id, timeStamp);
        default:
            break;
        }

        return storage.fetchImage(id, timeStamp);
    };

    const auto entry = requestImageFromStorage(requestType);

    if (entry) {
        if (entry->isNull()) {
            storageTraceToken.tick("there was an null image in storage"_t);
            abortCallback(ImageCache::AbortReason::Failed);
        } else {
            captureCallback(*entry);
            storageTraceToken.end(keyValue("storage id", id), keyValue("image", *entry));
        }
    } else {
        storageTraceToken.end();
        auto imageGeneratedCallback =
            [captureCallback = std::move(captureCallback),
             requestType](const QImage &image,
                          const QImage &midSizeImage,
                          const QImage &smallImage,
                          ImageCache::TraceToken traceToken) {
                auto token = traceToken.beginDuration("call capture callback"_t);
                auto selectImage = [](RequestType requestType,
                                      const QImage &image,
                                      const QImage &midSizeImage,
                                      const QImage &smallImage) {
                    switch (requestType) {
                    case RequestType::Image:
                        return image;
                    case RequestType::MidSizeImage:
                        return midSizeImage;
                    case RequestType::SmallImage:
                        return smallImage;
                    default:
                        break;
                    }

                    return image;
                };
                captureCallback(selectImage(requestType, image, midSizeImage, smallImage));
            };

        auto imageGenerationAbortedCallback =
            [abortCallback = std::move(abortCallback)](ImageCache::AbortReason reason,
                                                       ImageCache::TraceToken traceToken) {
                traceToken.tick("image could not be created"_t);
                abortCallback(reason);
            };

        traceToken.tick("call the generator"_t);

        generator.generateImage(name,
                                extraId,
                                timeStamp,
                                std::move(imageGeneratedCallback),
                                std::move(imageGenerationAbortedCallback),
                                std::move(auxiliaryData),
                                std::move(flowToken));
    }
}

void AsynchronousImageCache::requestImage(Utils::SmallStringView name,
                                          ImageCache::CaptureImageCallback captureCallback,
                                          ImageCache::AbortCallback abortCallback,
                                          Utils::SmallStringView extraId,
                                          ImageCache::AuxiliaryData auxiliaryData)
{
    auto [trace, flowToken] = ImageCache::category().beginDurationWithFlow(
        "request image in asynchronous image cache"_t);
    m_taskQueue.addTask(trace.createToken(),
                        std::move(name),
                        std::move(extraId),
                        std::move(captureCallback),
                        std::move(abortCallback),
                        std::move(auxiliaryData),
                        RequestType::Image,
                        std ::move(flowToken));
}

void AsynchronousImageCache::requestMidSizeImage(Utils::SmallStringView name,
                                                 ImageCache::CaptureImageCallback captureCallback,
                                                 ImageCache::AbortCallback abortCallback,
                                                 Utils::SmallStringView extraId,
                                                 ImageCache::AuxiliaryData auxiliaryData)
{
    auto [traceToken, flowToken] = ImageCache::category().beginDurationWithFlow(
        "request mid size image in asynchronous image cache"_t);
    m_taskQueue.addTask(traceToken.createToken(),
                        std::move(name),
                        std::move(extraId),
                        std::move(captureCallback),
                        std::move(abortCallback),
                        std::move(auxiliaryData),
                        RequestType::MidSizeImage,
                        std ::move(flowToken));
}

void AsynchronousImageCache::requestSmallImage(Utils::SmallStringView name,
                                               ImageCache::CaptureImageCallback captureCallback,
                                               ImageCache::AbortCallback abortCallback,
                                               Utils::SmallStringView extraId,
                                               ImageCache::AuxiliaryData auxiliaryData)
{
    auto [traceToken, flowtoken] = ImageCache::category().beginDurationWithFlow(
        "request small size image in asynchronous image cache"_t);
    m_taskQueue.addTask(traceToken.createToken(),
                        std::move(name),
                        std::move(extraId),
                        std::move(captureCallback),
                        std::move(abortCallback),
                        std::move(auxiliaryData),
                        RequestType::SmallImage,
                        std ::move(flowtoken));
}

void AsynchronousImageCache::clean()
{
    m_generator.clean();
    m_taskQueue.clean();
}

void AsynchronousImageCache::Dispatch::operator()(Entry &entry)
{
    using namespace NanotraceHR::Literals;

    request(entry.name,
            entry.extraId,
            entry.requestType,
            std::move(entry.captureCallback),
            std::move(entry.abortCallback),
            std::move(entry.auxiliaryData),
            std::move(entry.traceToken),
            storage,
            generator,
            timeStampProvider);
}

void AsynchronousImageCache::Clean::operator()(Entry &entry)
{
    using namespace NanotraceHR::Literals;

    entry.traceToken.tick("cleaning up in the cache"_t);

    entry.abortCallback(ImageCache::AbortReason::Abort);
}

} // namespace QmlDesigner
