// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "dapengine.h"

namespace Debugger::Internal {

class GdbDapEngine : public DapEngine
{
public:
    GdbDapEngine();

private:
    void setupEngine() override;

    void handleDapInitialize() override;
    void handleDapConfigurationDone() override;

    bool isLocalAttachEngine() const;

    const QLoggingCategory &logCategory() override;
};

} // Debugger::Internal
