// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "idevicefactory.h"

namespace ProjectExplorer {
namespace Internal {

class DesktopDeviceFactory final : public IDeviceFactory
{
public:
    DesktopDeviceFactory();
};

} // namespace Internal
} // namespace ProjectExplorer
