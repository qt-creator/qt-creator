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

    std::pair<QImage, QImage> createImage(Utils::SmallStringView filePath,
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
                       AbortCallback)
    {
        qWarning() << "ImageCacheDispatchCollector: cannot handle file type.";
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
    std::pair<QImage, QImage> dispatchCreateImage(Utils::SmallStringView filePath,
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

    std::pair<QImage, QImage> dispatchCreateImage(Utils::SmallStringView,
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
