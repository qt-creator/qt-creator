// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "asynchronousimagecacheinterface.h"

#include <imagecache/taskqueue.h>
#include <nanotrace/nanotracehr.h>

#include <condition_variable>
#include <deque>
#include <functional>
#include <mutex>
#include <optional>
#include <thread>

namespace QmlDesigner {

class TimeStampProviderInterface;
class ImageCacheStorageInterface;
class ImageCacheGeneratorInterface;
class ImageCacheCollectorInterface;

constexpr bool imageCacheTracingIsEnabled()
{
#ifdef ENABLE_IMAGE_CACHE_TRACING
    return NanotraceHR::isTracerActive();
#else
    return false;
#endif
}

using ImageCacheTraceToken = NanotraceHR::Token<imageCacheTracingIsEnabled()>;

NanotraceHR::StringViewCategory<imageCacheTracingIsEnabled()> &imageCacheCategory();

class AsynchronousImageCache final : public AsynchronousImageCacheInterface
{
public:
    ~AsynchronousImageCache();

    AsynchronousImageCache(ImageCacheStorageInterface &storage,
                           ImageCacheGeneratorInterface &generator,
                           TimeStampProviderInterface &timeStampProvider);

    void requestImage(Utils::SmallStringView name,
                      ImageCache::CaptureImageCallback captureCallback,
                      ImageCache::AbortCallback abortCallback,
                      Utils::SmallStringView extraId = {},
                      ImageCache::AuxiliaryData auxiliaryData = {}) override;
    void requestMidSizeImage(Utils::SmallStringView name,
                             ImageCache::CaptureImageCallback captureCallback,
                             ImageCache::AbortCallback abortCallback,
                             Utils::SmallStringView extraId = {},
                             ImageCache::AuxiliaryData auxiliaryData = {}) override;
    void requestSmallImage(Utils::SmallStringView name,
                           ImageCache::CaptureImageCallback captureCallback,
                           ImageCache::AbortCallback abortCallback,
                           Utils::SmallStringView extraId = {},
                           ImageCache::AuxiliaryData auxiliaryData = {}) override;

    void clean();

private:
    enum class RequestType { Image, MidSizeImage, SmallImage, Icon };
    struct Entry
    {
        Entry() = default;

        Entry(Utils::PathString name,
              Utils::SmallString extraId,
              ImageCache::CaptureImageCallback &&captureCallback,
              ImageCache::AbortCallback &&abortCallback,
              ImageCache::AuxiliaryData &&auxiliaryData,
              RequestType requestType,
              ImageCacheTraceToken traceToken)
            : name{std::move(name)}
            , extraId{std::move(extraId)}
            , captureCallback{std::move(captureCallback)}
            , abortCallback{std::move(abortCallback)}
            , auxiliaryData{std::move(auxiliaryData)}
            , requestType{requestType}
            , traceToken{traceToken}
        {}

        Utils::PathString name;
        Utils::SmallString extraId;
        ImageCache::CaptureImageCallback captureCallback;
        ImageCache::AbortCallback abortCallback;
        ImageCache::AuxiliaryData auxiliaryData;
        RequestType requestType = RequestType::Image;
        ImageCacheTraceToken traceToken;
    };

    static void request(Utils::SmallStringView name,
                        Utils::SmallStringView extraId,
                        AsynchronousImageCache::RequestType requestType,
                        ImageCache::CaptureImageCallback captureCallback,
                        ImageCache::AbortCallback abortCallback,
                        ImageCache::AuxiliaryData auxiliaryData,
                        ImageCacheTraceToken traceToken,
                        ImageCacheStorageInterface &storage,
                        ImageCacheGeneratorInterface &generator,
                        TimeStampProviderInterface &timeStampProvider);

    struct Dispatch
    {
        void operator()(Entry &entry)
        {
            request(entry.name,
                    entry.extraId,
                    entry.requestType,
                    std::move(entry.captureCallback),
                    std::move(entry.abortCallback),
                    std::move(entry.auxiliaryData),
                    entry.traceToken,
                    storage,
                    generator,
                    timeStampProvider);
        }

        ImageCacheStorageInterface &storage;
        ImageCacheGeneratorInterface &generator;
        TimeStampProviderInterface &timeStampProvider;
    };

    struct Clean
    {
        void operator()(Entry &entry)
        {
            using namespace NanotraceHR::Literals;

            entry.abortCallback(ImageCache::AbortReason::Abort);
            imageCacheCategory().endAsynchronous(entry.traceToken, "aborted for cleanup"_t);
        }
    };

private:
    ImageCacheStorageInterface &m_storage;
    ImageCacheGeneratorInterface &m_generator;
    TimeStampProviderInterface &m_timeStampProvider;
    TaskQueue<Entry, Dispatch, Clean> m_taskQueue{Dispatch{m_storage, m_generator, m_timeStampProvider},
                                                  Clean{}};
};

} // namespace QmlDesigner
