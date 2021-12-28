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

#include <utils/span.h>
#include <utils/variant.h>

#include <QImage>
#include <QSize>
#include <QString>

#include <functional>

namespace QmlDesigner {

namespace ImageCache {

class FontCollectorSizeAuxiliaryData
{
public:
    QSize size;
    QString colorName;
    QString text;
};

class FontCollectorSizesAuxiliaryData
{
public:
    Utils::span<const QSize> sizes;
    QString colorName;
    QString text;
};

using AuxiliaryData = Utils::variant<Utils::monostate, FontCollectorSizeAuxiliaryData, FontCollectorSizesAuxiliaryData>;

enum class AbortReason : char { Abort, Failed };

using CaptureImageCallback = std::function<void(const QImage &)>;
using CaptureImageWithSmallImageCallback = std::function<void(const QImage &image, const QImage &smallImage)>;
using AbortCallback = std::function<void(ImageCache::AbortReason)>;
} // namespace ImageCache

} // namespace QmlDesigner
