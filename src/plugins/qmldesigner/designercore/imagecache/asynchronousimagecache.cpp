// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "asynchronousimagecache.h"

#include "imagecachegenerator.h"
#include "imagecachestorage.h"
#include "timestampprovider.h"

#include <QScopeGuard>

#include <thread>

namespace QmlDesigner {

using namespace NanotraceHR::Literals;

namespace {
using TraceFile = NanotraceHR::TraceFile<imageCacheTracingIsEnabled()>;

TraceFile traceFile{"qml_designer.json"};

thread_local auto eventQueueData = NanotraceHR::makeEventQueueData<NanotraceHR::StringViewTraceEvent, 10000>(
    traceFile);
thread_local NanotraceHR::EventQueue eventQueue = eventQueueData.createEventQueue();

thread_local NanotraceHR::StringViewCategory<imageCacheTracingIsEnabled()> category{"image cache"_t,
                                                                                    eventQueue};
} // namespace

NanotraceHR::StringViewCategory<imageCacheTracingIsEnabled()> &imageCacheCategory()
{
    return category;
}

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
                                     ImageCacheTraceToken traceToken,
                                     ImageCacheStorageInterface &storage,
                                     ImageCacheGeneratorInterface &generator,
                                     TimeStampProviderInterface &timeStampProvider)
{
    const auto id = extraId.empty() ? Utils::PathString{name}
                                    : Utils::PathString::join({name, "+", extraId});

    const auto timeStamp = timeStampProvider.timeStamp(name);
    auto requestImageFromStorage = [&](RequestType requestType) {
        imageCacheCategory().tickAsynchronous(traceToken, "start fetching image from storage"_t);

        QScopeGuard finally{[=] {
            imageCacheCategory().tickAsynchronous(traceToken, "end fetching image from storage"_t);
        }};
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
            abortCallback(ImageCache::AbortReason::Failed);
            imageCacheCategory().endAsynchronous(traceToken,
                                                 "abort image request because entry in database is null"_t);
        } else {
            captureCallback(*entry);
            imageCacheCategory().endAsynchronous(traceToken, "image request delivered from storage"_t);
        }
    } else {
        auto callback =
            [captureCallback = std::move(captureCallback),
             requestType,
             traceToken](const QImage &image, const QImage &midSizeImage, const QImage &smallImage) {
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
                imageCacheCategory().endAsynchronous(traceToken,
                                                     "image request delivered from generation"_t);
            };

        imageCacheCategory().tickAsynchronous(traceToken, "request image generation"_t);

        generator.generateImage(name,
                                extraId,
                                timeStamp,
                                std::move(callback),
                                std::move(abortCallback),
                                std::move(auxiliaryData));
    }
}

void AsynchronousImageCache::requestImage(Utils::SmallStringView name,
                                          ImageCache::CaptureImageCallback captureCallback,
                                          ImageCache::AbortCallback abortCallback,
                                          Utils::SmallStringView extraId,
                                          ImageCache::AuxiliaryData auxiliaryData)
{
    auto traceToken = imageCacheCategory().beginAsynchronous(
        "request image in asynchornous image cache"_t);
    m_taskQueue.addTask(std::move(name),
                        std::move(extraId),
                        std::move(captureCallback),
                        std::move(abortCallback),
                        std::move(auxiliaryData),
                        RequestType::Image,
                        traceToken);
}

void AsynchronousImageCache::requestMidSizeImage(Utils::SmallStringView name,
                                                 ImageCache::CaptureImageCallback captureCallback,
                                                 ImageCache::AbortCallback abortCallback,
                                                 Utils::SmallStringView extraId,
                                                 ImageCache::AuxiliaryData auxiliaryData)
{
    auto traceToken = imageCacheCategory().beginAsynchronous(
        "request mid size image in asynchornous image cache"_t);
    m_taskQueue.addTask(std::move(name),
                        std::move(extraId),
                        std::move(captureCallback),
                        std::move(abortCallback),
                        std::move(auxiliaryData),
                        RequestType::MidSizeImage,
                        traceToken);
}

void AsynchronousImageCache::requestSmallImage(Utils::SmallStringView name,
                                               ImageCache::CaptureImageCallback captureCallback,
                                               ImageCache::AbortCallback abortCallback,
                                               Utils::SmallStringView extraId,
                                               ImageCache::AuxiliaryData auxiliaryData)
{
    auto traceToken = imageCacheCategory().beginAsynchronous(
        "request small size image in asynchornous image cache"_t);
    m_taskQueue.addTask(std::move(name),
                        std::move(extraId),
                        std::move(captureCallback),
                        std::move(abortCallback),
                        std::move(auxiliaryData),
                        RequestType::SmallImage,
                        traceToken);
}

void AsynchronousImageCache::clean()
{
    m_generator.clean();
    m_taskQueue.clean();
}

} // namespace QmlDesigner
