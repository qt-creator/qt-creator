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
#include <projectexplorer/runconfigurationaspects.h>

namespace Debugger {

namespace Internal { class DebuggerLanguageAspect; }

class DEBUGGER_EXPORT DebuggerRunConfigurationAspect
    : public ProjectExplorer::GlobalOrProjectAspect
{
    Q_OBJECT

public:
    DebuggerRunConfigurationAspect(ProjectExplorer::Target *target);
    ~DebuggerRunConfigurationAspect();

    void fromMap(const QVariantMap &map) override;
    void toMap(QVariantMap &map) const override;

    bool useCppDebugger() const;
    bool useQmlDebugger() const;
    void setUseQmlDebugger(bool value);
    bool useMultiProcess() const;
    void setUseMultiProcess(bool on);
    QString overrideStartup() const;

    int portsUsedByDebugger() const;

private:
    Internal::DebuggerLanguageAspect *m_cppAspect;
    Internal::DebuggerLanguageAspect *m_qmlAspect;
    ProjectExplorer::BaseBoolAspect *m_multiProcessAspect;
    ProjectExplorer::BaseStringAspect *m_overrideStartupAspect;
    ProjectExplorer::Target *m_target;
};

} // namespace Debugger
