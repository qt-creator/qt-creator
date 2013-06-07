/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
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
    : public QObject, public ProjectExplorer::IRunConfigurationAspect
{
    Q_OBJECT

public:
    DebuggerRunConfigurationAspect(ProjectExplorer::RunConfiguration *runConfiguration);
    DebuggerRunConfigurationAspect(ProjectExplorer::RunConfiguration *runConfiguration,
                                   const DebuggerRunConfigurationAspect *other);

    enum DebuggerLanguageStatus {
        DisabledLanguage = 0,
        EnabledLanguage,
        AutoEnabledLanguage
    };

    QVariantMap toMap() const;
    void fromMap(const QVariantMap &map);

    DebuggerRunConfigurationAspect *clone(ProjectExplorer::RunConfiguration *parent) const;
    ProjectExplorer::RunConfigWidget *createConfigurationWidget();

    QString displayName() const;

    bool useCppDebugger() const;
    void setUseCppDebugger(bool value);
    bool useQmlDebugger() const;
    void setUseQmlDebugger(bool value);
    uint qmlDebugServerPort() const;
    void setQmllDebugServerPort(uint port);
    bool useMultiProcess() const;
    void setUseMultiProcess(bool on);
    bool isQmlDebuggingSpinboxSuppressed() const;
    ProjectExplorer::RunConfiguration *runConfiguration();

signals:
    void debuggersChanged();

private:
    void ctor();

    ProjectExplorer::RunConfiguration *m_runConfiguration;
    DebuggerLanguageStatus m_useCppDebugger;
    DebuggerLanguageStatus m_useQmlDebugger;
    uint m_qmlDebugServerPort;
    bool m_useMultiProcess;

    friend class Internal::DebuggerRunConfigWidget;
};

} // namespace Debugger

#endif // DEBUGGERRUNCONFIGURATIONASPECT_H
