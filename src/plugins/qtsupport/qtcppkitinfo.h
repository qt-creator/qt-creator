// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qtsupport_global.h"

#include <projectexplorer/rawprojectpart.h>

namespace QtSupport {

class QtVersion;

class QTSUPPORT_EXPORT CppKitInfo : public ProjectExplorer::KitInfo
{
public:
    CppKitInfo(ProjectExplorer::Kit *kit);

    QtVersion *qtVersion = nullptr;
};

} // namespace QtSupport
