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
    void setupEngine() override;

    /* Needed for CMake support issue:25176 */
    void insertBreakpoint(const Breakpoint &bp) override;
    void updateBreakpoint(const Breakpoint &bp) override;
    void removeBreakpoint(const Breakpoint &bp) override;
    /* Needed for CMake support issue:25176 */

    bool hasCapability(unsigned cap) const override;

    const QLoggingCategory &logCategory() override;
};

} // Debugger::Internal
