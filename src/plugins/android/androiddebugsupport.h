// Copyright (C) 2016 BogDan Vatra <bog_dan_ro@yahoo.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/runcontrol.h>

namespace Android::Internal {

class AndroidDebugWorkerFactory final : public ProjectExplorer::RunWorkerFactory
{
public:
    AndroidDebugWorkerFactory();
};

} // Android::Internal
