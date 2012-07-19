/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
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

#ifndef REMOTELINUXQMLPROFILERRUNNER_H
#define REMOTELINUXQMLPROFILERRUNNER_H

#include "abstractqmlprofilerrunner.h"
#include <remotelinux/remotelinuxrunconfiguration.h>
#include <remotelinux/remotelinuxruncontrol.h>

namespace QmlProfiler {
namespace Internal {

class RemoteLinuxQmlProfilerRunner : public AbstractQmlProfilerRunner
{
    Q_OBJECT

    using AbstractQmlProfilerRunner::appendMessage; // don't hide signal
public:
    explicit RemoteLinuxQmlProfilerRunner(RemoteLinux::RemoteLinuxRunConfiguration *configuration,
                                   QObject *parent = 0);
    ~RemoteLinuxQmlProfilerRunner();

    // AbstractQmlProfilerRunner
    virtual void start();
    virtual void stop();
    virtual quint16 debugPort() const;

private slots:
    void getPorts();
    void handleError(const QString &msg);
    void handleStdErr(const QByteArray &msg);
    void handleStdOut(const QByteArray &msg);
    void handleRemoteProcessStarted();
    void handleRemoteProcessFinished(qint64);
    void handleProgressReport(const QString &progressString);

private:
    RemoteLinux::AbstractRemoteLinuxApplicationRunner *runner() const;

    quint16 m_port;
    RemoteLinux::AbstractRemoteLinuxRunControl *m_runControl;
};

} // namespace Internal
} // namespace QmlProfiler

#endif // REMOTELINUXQMLPROFILERRUNNER_H
