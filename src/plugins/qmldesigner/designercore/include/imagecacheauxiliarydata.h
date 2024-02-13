// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <nanotrace/nanotracehr.h>
#include <utils/span.h>

#include <QImage>
#include <QSize>
#include <QString>

#include <functional>
#include <variant>

namespace QmlDesigner {

namespace ImageCache {

constexpr NanotraceHR::Tracing tracingStatus()
{
#ifdef ENABLE_IMAGE_CACHE_TRACING
    return NanotraceHR::Tracing::IsEnabled;
#else
    return NanotraceHR::Tracing::IsDisabled;
#endif
}

using Category = NanotraceHR::StringViewWithStringArgumentsCategory<tracingStatus()>;
using TraceToken = Category::FlowTokenType;
using FlowToken = Category::FlowTokenType;
using Token = Category::TokenType;
extern Category &category();

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

class LibraryIconAuxiliaryData
{
public:
    bool enable;
};

using AuxiliaryData = std::variant<std::monostate,
                                   LibraryIconAuxiliaryData,
                                   FontCollectorSizeAuxiliaryData,
                                   FontCollectorSizesAuxiliaryData>;

enum class AbortReason : char { Abort, Failed, NoEntry };

using CaptureImageCallback = std::function<void(const QImage &)>;
using CaptureImageWithScaledImagesCallback = std::function<void(
    const QImage &image, const QImage &midSizeImage, const QImage &smallImage, ImageCache::TraceToken)>;
using AbortCallback = std::function<void(ImageCache::AbortReason)>;
using InternalAbortCallback = std::function<void(ImageCache::AbortReason, ImageCache::TraceToken)>;

} // namespace ImageCache

} // namespace QmlDesigner
