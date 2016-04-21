/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include "debugger_global.h"
#include "debuggerconstants.h"

#include <projectexplorer/runconfiguration.h>

namespace Debugger {

namespace Internal { class DebuggerRunConfigWidget;  }

enum DebuggerLanguageStatus {
    DisabledLanguage = 0,
    EnabledLanguage,
    AutoEnabledLanguage
};

class DEBUGGER_EXPORT DebuggerRunConfigurationAspectData
{
public:
    DebuggerLanguageStatus useCppDebugger = AutoEnabledLanguage;
    DebuggerLanguageStatus useQmlDebugger = AutoEnabledLanguage;
    uint qmlDebugServerPort = Constants::QML_DEFAULT_DEBUG_SERVER_PORT;
    bool useMultiProcess = false;
};

class DEBUGGER_EXPORT DebuggerRunConfigurationAspect
    : public ProjectExplorer::IRunConfigurationAspect
{
    Q_OBJECT

public:
    DebuggerRunConfigurationAspect(ProjectExplorer::RunConfiguration *runConfiguration);
    DebuggerRunConfigurationAspect *create(ProjectExplorer::RunConfiguration *runConfiguration) const;

    void fromMap(const QVariantMap &map);
    void toMap(QVariantMap &map) const;

    bool useCppDebugger() const;
    void setUseCppDebugger(bool value);
    bool useQmlDebugger() const;
    void setUseQmlDebugger(bool value);
    uint qmlDebugServerPort() const;
    void setQmllDebugServerPort(uint port);
    bool useMultiProcess() const;
    void setUseMultiProcess(bool on);
    bool isQmlDebuggingSpinboxSuppressed() const;

    int portsUsedByDebugger() const;

private:
    friend class Internal::DebuggerRunConfigWidget;
    DebuggerRunConfigurationAspectData d;
};

} // namespace Debugger
