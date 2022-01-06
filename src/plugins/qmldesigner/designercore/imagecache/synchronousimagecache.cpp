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

#include "synchronousimagecache.h"

#include "imagecachecollectorinterface.h"
#include "imagecachestorage.h"
#include "timestampprovider.h"

#include <thread>

namespace QmlDesigner {

namespace {

Utils::PathString createId(Utils::PathString filePath, Utils::SmallString extraId)
{
    return extraId.empty() ? Utils::PathString{filePath}
                           : Utils::PathString::join({filePath, "+", extraId});
}
} // namespace

QImage SynchronousImageCache::image(Utils::PathString filePath,
                                    Utils::SmallString extraId,
                                    const ImageCache::AuxiliaryData &auxiliaryData)
{
    const auto id = createId(filePath, extraId);

    const auto timeStamp = m_timeStampProvider.timeStamp(filePath);
    const auto entry = m_storage.fetchImage(id, timeStamp);

    if (entry)
        return *entry;

    const auto &[image, smallImage] = m_collector.createImage(filePath, extraId, auxiliaryData);

    m_storage.storeImage(id, timeStamp, image, smallImage);

    return image;
}

QImage SynchronousImageCache::smallImage(Utils::PathString filePath,
                                         Utils::SmallString extraId,
                                         const ImageCache::AuxiliaryData &auxiliaryData)
{
    const auto id = createId(filePath, extraId);

    const auto timeStamp = m_timeStampProvider.timeStamp(filePath);
    const auto entry = m_storage.fetchSmallImage(id, timeStamp);

    if (entry)
        return *entry;

    const auto &[image, smallImage] = m_collector.createImage(filePath, extraId, auxiliaryData);

    m_storage.storeImage(id, timeStamp, image, smallImage);

    return smallImage;
}

QIcon SynchronousImageCache::icon(Utils::PathString filePath,
                                  Utils::SmallString extraId,
                                  const ImageCache::AuxiliaryData &auxiliaryData)
{
    const auto id = createId(filePath, extraId);

    const auto timeStamp = m_timeStampProvider.timeStamp(filePath);
    const auto entry = m_storage.fetchIcon(id, timeStamp);

    if (entry)
        return *entry;

    const auto icon = m_collector.createIcon(filePath, extraId, auxiliaryData);

    m_storage.storeIcon(id, timeStamp, icon);

    return icon;
}

} // namespace QmlDesigner
