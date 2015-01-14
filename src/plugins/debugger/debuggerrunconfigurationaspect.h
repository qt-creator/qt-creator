/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef DEBUGGERRUNCONFIGURATIONASPECT_H
#define DEBUGGERRUNCONFIGURATIONASPECT_H

#include "debugger_global.h"

#include <projectexplorer/runconfiguration.h>

namespace Debugger {

namespace Internal { class DebuggerRunConfigWidget;  }

class DEBUGGER_EXPORT DebuggerRunConfigurationAspect
    : public ProjectExplorer::IRunConfigurationAspect
{
    Q_OBJECT

public:
    DebuggerRunConfigurationAspect(ProjectExplorer::RunConfiguration *runConfiguration);
    DebuggerRunConfigurationAspect *create(ProjectExplorer::RunConfiguration *runConfiguration) const;

    enum DebuggerLanguageStatus {
        DisabledLanguage = 0,
        EnabledLanguage,
        AutoEnabledLanguage
    };

    void fromMap(const QVariantMap &map);
    void toMap(QVariantMap &map) const;

    ProjectExplorer::RunConfigWidget *createConfigurationWidget();

    bool useCppDebugger() const;
    void setUseCppDebugger(bool value);
    bool useQmlDebugger() const;
    void setUseQmlDebugger(bool value);
    uint qmlDebugServerPort() const;
    void setQmllDebugServerPort(uint port);
    bool useMultiProcess() const;
    void setUseMultiProcess(bool on);
    bool isQmlDebuggingSpinboxSuppressed() const;

private:
    DebuggerLanguageStatus m_useCppDebugger;
    DebuggerLanguageStatus m_useQmlDebugger;
    uint m_qmlDebugServerPort;
    bool m_useMultiProcess;

    friend class Internal::DebuggerRunConfigWidget;
};

} // namespace Debugger

#endif // DEBUGGERRUNCONFIGURATIONASPECT_H
