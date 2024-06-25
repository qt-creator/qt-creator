// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "dapengine.h"

namespace Debugger::Internal {

class LldbDapEngine : public DapEngine
{
public:
    LldbDapEngine();

private:
    void setupEngine() override;

    void handleDapInitialize() override;
    void handleDapConfigurationDone() override;

    bool isLocalAttachEngine() const;
    bool acceptsBreakpoint(const BreakpointParameters &bp) const override;
    const QLoggingCategory &logCategory() override;

    QJsonArray sourceMap() const;
    QJsonArray preRunCommands() const;
};

} // Debugger::Internal
