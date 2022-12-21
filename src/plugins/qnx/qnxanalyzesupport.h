// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/runcontrol.h>

namespace Qnx::Internal {

class QnxQmlProfilerSupport : public ProjectExplorer::SimpleTargetRunner
{
public:
    explicit QnxQmlProfilerSupport(ProjectExplorer::RunControl *runControl);
};

} // Qnx::Internal
