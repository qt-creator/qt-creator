// Copyright (C) 2016 BogDan Vatra <bog_dan_ro@yahoo.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "androidrunner.h"

#include <projectexplorer/runconfiguration.h>

namespace Android {
namespace Internal {

class AndroidRunner;

class AndroidRunSupport final : public AndroidRunner
{
    Q_OBJECT

public:
    explicit AndroidRunSupport(ProjectExplorer::RunControl *runControl,
                               const QString &intentName = QString());
    ~AndroidRunSupport() override;

    void start() override;
    void stop() override;
};

} // namespace Internal
} // namespace Android
