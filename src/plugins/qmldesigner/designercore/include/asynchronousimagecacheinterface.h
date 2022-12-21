// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "imagecacheauxiliarydata.h"

#include <utils/smallstring.h>

#include <QImage>

namespace QmlDesigner {

class AsynchronousImageCacheInterface
{
public:
    virtual void requestImage(Utils::PathString name,
                              ImageCache::CaptureImageCallback captureCallback,
                              ImageCache::AbortCallback abortCallback,
                              Utils::SmallString extraId = {},
                              ImageCache::AuxiliaryData auxiliaryData = {})
        = 0;
    virtual void requestSmallImage(Utils::PathString name,
                                   ImageCache::CaptureImageCallback captureCallback,
                                   ImageCache::AbortCallback abortCallback,
                                   Utils::SmallString extraId = {},
                                   ImageCache::AuxiliaryData auxiliaryData = {})
        = 0;

    void clean();

protected:
    ~AsynchronousImageCacheInterface() = default;
};

} // namespace QmlDesigner
