// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "appinfo.h"

Q_GLOBAL_STATIC(Utils::AppInfo, sAppInfo)

namespace Utils {

const Utils::AppInfo &appInfo()
{
    return *sAppInfo;
}

void Internal::setAppInfo(const AppInfo &info)
{
    *sAppInfo = info;
}

} // namespace Utils
