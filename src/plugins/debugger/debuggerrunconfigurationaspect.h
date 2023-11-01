// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "debugger_global.h"

#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/runconfigurationaspects.h>

namespace Debugger {

class DEBUGGER_EXPORT DebuggerRunConfigurationAspect
    : public ProjectExplorer::GlobalOrProjectAspect
{
public:
    DebuggerRunConfigurationAspect(ProjectExplorer::Target *target);
    ~DebuggerRunConfigurationAspect();

    void fromMap(const Utils::Store &map) override;
    void toMap(Utils::Store &map) const override;

    bool useCppDebugger() const;
    bool useQmlDebugger() const;
    void setUseQmlDebugger(bool value);
    bool useMultiProcess() const;
    void setUseMultiProcess(bool on);
    QString overrideStartup() const;

    int portsUsedByDebugger() const;

    struct Data : BaseAspect::Data
    {
        bool useCppDebugger;
        bool useQmlDebugger;
        bool useMultiProcess;
        QString overrideStartup;
    };

private:
    Utils::TriStateAspect *m_cppAspect;
    Utils::TriStateAspect *m_qmlAspect;
    Utils::BoolAspect *m_multiProcessAspect;
    Utils::StringAspect *m_overrideStartupAspect;
    ProjectExplorer::Target *m_target;
};

} // namespace Debugger
