// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/runcontrol.h>

namespace Android::Internal {

class AndroidQmlPreviewWorkerFactory final : public ProjectExplorer::RunWorkerFactory
{
public:
    AndroidQmlPreviewWorkerFactory();
};

} // Android::Internal
