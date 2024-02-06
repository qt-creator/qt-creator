// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <memory>

namespace ProjectExplorer {

class IDevice;

using IDevicePtr = std::shared_ptr<IDevice>;
using IDeviceConstPtr = std::shared_ptr<const IDevice>;

} // namespace ProjectExplorer
