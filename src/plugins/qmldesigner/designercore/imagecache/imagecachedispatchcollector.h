// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "imagecachecollectorinterface.h"

namespace QmlDesigner {

template<typename CollectorEntries>
class ImageCacheDispatchCollector final : public ImageCacheCollectorInterface
{
public:
    ImageCacheDispatchCollector(CollectorEntries collectors)
        : m_collectors{std::move(collectors)} {};

    void start(Utils::SmallStringView filePath,
               Utils::SmallStringView state,
               const ImageCache::AuxiliaryData &auxiliaryData,
               CaptureCallback captureCallback,
               AbortCallback abortCallback) override
    {
        std::apply(
            [&](const auto &...collectors) {
                dispatchStart(filePath,
                              state,
                              auxiliaryData,
                              std::move(captureCallback),
                              std::move(abortCallback),
                              collectors...);
            },
            m_collectors);
    }

    ImageTuple createImage(Utils::SmallStringView filePath,
                           Utils::SmallStringView state,
                           const ImageCache::AuxiliaryData &auxiliaryData) override
    {
        return std::apply(
            [&](const auto &...entries) {
                return dispatchCreateImage(filePath, state, auxiliaryData, entries...);
            },
            m_collectors);
    }

    QIcon createIcon(Utils::SmallStringView filePath,
                     Utils::SmallStringView state,
                     const ImageCache::AuxiliaryData &auxiliaryData) override
    {
        return std::apply(
            [&](const auto &...entries) {
                return dispatchCreateIcon(filePath, state, auxiliaryData, entries...);
            },
            m_collectors);
    }

private:
    template<typename Collector, typename... Collectors>
    void dispatchStart(Utils::SmallStringView filePath,
                       Utils::SmallStringView state,
                       const ImageCache::AuxiliaryData &auxiliaryData,
                       CaptureCallback captureCallback,
                       AbortCallback abortCallback,
                       const Collector &collector,
                       const Collectors &...collectors)
    {
        if (collector.first(filePath, state, auxiliaryData)) {
            collector.second->start(filePath,
                                    state,
                                    auxiliaryData,
                                    std::move(captureCallback),
                                    std::move(abortCallback));
        } else {
            dispatchStart(filePath,
                          state,
                          auxiliaryData,
                          std::move(captureCallback),
                          std::move(abortCallback),
                          collectors...);
        }
    }

    void dispatchStart(Utils::SmallStringView,
                       Utils::SmallStringView,
                       const ImageCache::AuxiliaryData &,
                       CaptureCallback,
                       AbortCallback abortCallback)
    {
        qWarning() << "ImageCacheDispatchCollector: cannot handle file type.";
        abortCallback(ImageCache::AbortReason::Failed);
    }

    template<typename Collector, typename... Collectors>
    QIcon dispatchCreateIcon(Utils::SmallStringView filePath,
                             Utils::SmallStringView state,
                             const ImageCache::AuxiliaryData &auxiliaryData,
                             const Collector &collector,
                             const Collectors &...collectors)
    {
        if (collector.first(filePath, state, auxiliaryData)) {
            return collector.second->createIcon(filePath, state, auxiliaryData);
        }

        return dispatchCreateIcon(filePath, state, auxiliaryData, collectors...);
    }

    QIcon dispatchCreateIcon(Utils::SmallStringView,
                             Utils::SmallStringView,
                             const ImageCache::AuxiliaryData &)
    {
        qWarning() << "ImageCacheDispatchCollector: cannot handle file type.";

        return {};
    }

    template<typename Collector, typename... Collectors>
    ImageTuple dispatchCreateImage(Utils::SmallStringView filePath,
                                   Utils::SmallStringView state,
                                   const ImageCache::AuxiliaryData &auxiliaryData,
                                   const Collector &collector,
                                   const Collectors &...collectors)
    {
        if (collector.first(filePath, state, auxiliaryData)) {
            return collector.second->createImage(filePath, state, auxiliaryData);
        }

        return dispatchCreateImage(filePath, state, auxiliaryData, collectors...);
    }

    ImageTuple dispatchCreateImage(Utils::SmallStringView,
                                   Utils::SmallStringView,
                                   const ImageCache::AuxiliaryData &)
    {
        qWarning() << "ImageCacheDispatchCollector: cannot handle file type.";

        return {};
    }

private:
    CollectorEntries m_collectors;
};

} // namespace QmlDesigner
