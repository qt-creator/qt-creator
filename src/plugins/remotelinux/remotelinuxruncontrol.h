/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#ifndef REMOTELINUXRUNCONTROL_H
#define REMOTELINUXRUNCONTROL_H

#include "remotelinux_export.h"

#include <projectexplorer/runconfiguration.h>

QT_FORWARD_DECLARE_CLASS(QString)

namespace RemoteLinux {
class AbstractRemoteLinuxApplicationRunner;

class REMOTELINUX_EXPORT AbstractRemoteLinuxRunControl : public ProjectExplorer::RunControl
{
    Q_OBJECT
    Q_DISABLE_COPY(AbstractRemoteLinuxRunControl)
public:
    explicit AbstractRemoteLinuxRunControl(ProjectExplorer::RunConfiguration *runConfig);
    virtual ~AbstractRemoteLinuxRunControl();

    virtual void start();
    virtual StopResult stop();
    virtual bool isRunning() const;
    virtual QIcon icon() const;

    virtual AbstractRemoteLinuxApplicationRunner *runner() const = 0;

private slots:
    void startExecution();
    void handleSshError(const QString &error);
    void handleRemoteProcessStarted() {}
    void handleRemoteProcessFinished(qint64 exitCode);
    void handleRemoteOutput(const QByteArray &output);
    void handleRemoteErrorOutput(const QByteArray &output);
    void handleProgressReport(const QString &progressString);

private:
    void setFinished();
    void handleError(const QString &errString);

    bool m_running;
};


class REMOTELINUX_EXPORT RemoteLinuxRunControl : public AbstractRemoteLinuxRunControl
{
    Q_OBJECT

public:
    explicit RemoteLinuxRunControl(ProjectExplorer::RunConfiguration *runConfig);
    virtual ~RemoteLinuxRunControl();
private:
    virtual AbstractRemoteLinuxApplicationRunner *runner() const;

    AbstractRemoteLinuxApplicationRunner * const m_runner;
};

} // namespace RemoteLinux

#endif // REMOTELINUXRUNCONTROL_H
