// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/runconfiguration.h>

namespace QmlProfiler::Internal {

class QmlProfilerRunConfigurationAspect : public ProjectExplorer::GlobalOrProjectAspect
{
public:
    QmlProfilerRunConfigurationAspect(ProjectExplorer::Target *);
};

} // QmlProfiler::Internal
