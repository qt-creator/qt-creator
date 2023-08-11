// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "dapengine.h"

namespace Debugger::Internal {

class CMakeDapEngine : public DapEngine
{
public:
    CMakeDapEngine();

private:
    void handleDapStarted() override;
    void setupEngine() override;

    bool hasCapability(unsigned cap) const override;
};

} // Debugger::Internal
