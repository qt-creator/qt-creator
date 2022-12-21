// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/runcontrol.h>
#include <utils/environment.h>

namespace Android {
namespace Internal {

class AndroidQmlToolingSupport : public ProjectExplorer::RunWorker
{
    Q_OBJECT

public:
    explicit AndroidQmlToolingSupport(ProjectExplorer::RunControl *runControl,
                                      const QString &intentName = QString());

private:
    void start() override;
    void stop() override;
};

} // namespace Internal
} // namespace Android
