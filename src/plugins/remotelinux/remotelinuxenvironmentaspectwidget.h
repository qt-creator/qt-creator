// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/environmentaspectwidget.h>

namespace RemoteLinux {

class RemoteLinuxEnvironmentAspect;

class RemoteLinuxEnvironmentAspectWidget : public ProjectExplorer::EnvironmentAspectWidget
{
    Q_OBJECT

public:
    RemoteLinuxEnvironmentAspectWidget(RemoteLinuxEnvironmentAspect *aspect,
                                       ProjectExplorer::Target *target);
};

} // namespace RemoteLinux
