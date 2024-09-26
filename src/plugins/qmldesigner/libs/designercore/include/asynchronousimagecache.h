// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <qmldesignercorelib_exports.h>

#include "asynchronousimagecacheinterface.h"

#include <imagecache/taskqueue.h>
#include <qmldesignercorelib_exports.h>

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

class QMLDESIGNERCORE_EXPORT AsynchronousImageCache final : public AsynchronousImageCacheInterface
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
              ImageCache::TraceToken traceToken)
            : name{std::move(name)}
            , extraId{std::move(extraId)}
            , captureCallback{std::move(captureCallback)}
            , abortCallback{std::move(abortCallback)}
            , auxiliaryData{std::move(auxiliaryData)}
            , requestType{requestType}
            , traceToken{std::move(traceToken)}
        {}

        Utils::PathString name;
        Utils::SmallString extraId;
        ImageCache::CaptureImageCallback captureCallback;
        ImageCache::AbortCallback abortCallback;
        ImageCache::AuxiliaryData auxiliaryData;
        RequestType requestType = RequestType::Image;
        NO_UNIQUE_ADDRESS ImageCache::TraceToken traceToken;
    };

    static void request(Utils::SmallStringView name,
                        Utils::SmallStringView extraId,
                        AsynchronousImageCache::RequestType requestType,
                        ImageCache::CaptureImageCallback captureCallback,
                        ImageCache::AbortCallback abortCallback,
                        ImageCache::AuxiliaryData auxiliaryData,
                        ImageCache::TraceToken traceToken,
                        ImageCacheStorageInterface &storage,
                        ImageCacheGeneratorInterface &generator,
                        TimeStampProviderInterface &timeStampProvider);

    struct Dispatch
    {
        QMLDESIGNERCORE_EXPORT void operator()(Entry &entry);

        ImageCacheStorageInterface &storage;
        ImageCacheGeneratorInterface &generator;
        TimeStampProviderInterface &timeStampProvider;
    };

    struct Clean
    {
        QMLDESIGNERCORE_EXPORT void operator()(Entry &entry);
    };

private:
    ImageCacheStorageInterface &m_storage;
    ImageCacheGeneratorInterface &m_generator;
    TimeStampProviderInterface &m_timeStampProvider;
    TaskQueue<Entry, Dispatch, Clean> m_taskQueue{Dispatch{m_storage, m_generator, m_timeStampProvider},
                                                  Clean{}};
};

} // namespace QmlDesigner
