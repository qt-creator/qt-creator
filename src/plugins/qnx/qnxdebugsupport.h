// Copyright (C) 2016 BlackBerry Limited. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <debugger/debuggerruncontrol.h>

namespace Qnx::Internal {

void showAttachToProcessDialog();

class QnxDebugWorkerFactory final : public ProjectExplorer::RunWorkerFactory
{
public:
    QnxDebugWorkerFactory();
};

} // Qnx::Internal
