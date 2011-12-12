/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef SIMPLERUNNER_H
#define SIMPLERUNNER_H

#include "remotelinux_export.h"

#include <utils/ssh/sshconnection.h>

#include <QtCore/QObject>

QT_FORWARD_DECLARE_CLASS(QString)

namespace RemoteLinux {
class LinuxDeviceConfiguration;

namespace Internal {
class SimpleRunnerPrivate;
} // namespace Internal

// -----------------------------------------------------------------------
// SimpleRunner:
// -----------------------------------------------------------------------

class REMOTELINUX_EXPORT SimpleRunner : public QObject
{
    Q_OBJECT

public:
    SimpleRunner(const QSharedPointer<const LinuxDeviceConfiguration> &deviceConfiguration,
                 const QString &commandline);
    ~SimpleRunner();

    virtual QString commandLine() const;

    virtual void run();
    virtual void cancel();

signals:
    void aboutToStart();
    void started();
    void progressMessage(const QString &message);
    void errorMessage(const QString &message);
    // 0 on success, other value on failure.
    void finished(int result);

protected slots:
    void handleProcessFinished(int exitStatus);

    virtual void handleConnectionFailure();
    virtual void handleStdOutput(const QByteArray &data);
    virtual void handleStdError(const QByteArray &data);

    // 0 on success, any other value on failure.
    virtual int processFinished(int exitStatus);

protected:
    void setCommandLine(const QString &cmd);

    Utils::SshConnectionParameters sshParameters() const;

private:
    Internal::SimpleRunnerPrivate *const d;
};

} // namespace RemoteLinux

#endif // SIMPLERUNNER_H
