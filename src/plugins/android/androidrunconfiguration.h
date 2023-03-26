// Copyright (C) 2016 BogDan Vatra <bog_dan_ro@yahoo.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "android_global.h"

#include <projectexplorer/runconfiguration.h>

namespace Android {

class ANDROID_EXPORT AndroidRunConfiguration : public ProjectExplorer::RunConfiguration
{
    Q_OBJECT
public:
    explicit AndroidRunConfiguration(ProjectExplorer::Target *target, Utils::Id id);
};

} // namespace Android
