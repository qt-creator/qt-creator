/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#include <utils/smallstring.h>

#include <QIcon>
#include <QImage>

#include <condition_variable>
#include <functional>
#include <mutex>
#include <thread>

namespace QmlDesigner {

class TimeStampProviderInterface;
class ImageCacheStorageInterface;
class ImageCacheGeneratorInterface;
class ImageCacheCollectorInterface;

class SynchronousImageCache
{
public:
    using CaptureImageCallback = std::function<void(const QImage &)>;
    using AbortCallback = std::function<void()>;

    SynchronousImageCache(ImageCacheStorageInterface &storage,
                          TimeStampProviderInterface &timeStampProvider,
                          ImageCacheCollectorInterface &collector)
        : m_storage(storage)
        , m_timeStampProvider(timeStampProvider)
        , m_collector(collector)
    {}

    QImage image(Utils::PathString filePath,
                 Utils::SmallString extraId = {},
                 const ImageCache::AuxiliaryData &auxiliaryData = {});
    QImage smallImage(Utils::PathString filePath,
                      Utils::SmallString extraId = {},
                      const ImageCache::AuxiliaryData &auxiliaryData = {});
    QIcon icon(Utils::PathString filePath,
               Utils::SmallString extraId = {},
               const ImageCache::AuxiliaryData &auxiliaryData = {});

private:
    ImageCacheStorageInterface &m_storage;
    TimeStampProviderInterface &m_timeStampProvider;
    ImageCacheCollectorInterface &m_collector;
};

} // namespace QmlDesigner
