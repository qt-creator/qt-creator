/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Author: Milian Wolff, KDAB (milian.wolff@kdab.com)
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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

#ifndef VALGRIND_RUNNER_H
#define VALGRIND_RUNNER_H

#include <QtCore/QProcess>

#include "valgrind_global.h"

QT_BEGIN_NAMESPACE
class QProcessEnvironment;
QT_END_NAMESPACE

namespace Utils {
class Environment;
}

namespace Valgrind {

class VALGRINDSHARED_EXPORT ValgrindRunner : public QObject
{
    Q_OBJECT

public:
    explicit ValgrindRunner(QObject *parent = 0);
    ~ValgrindRunner();

    QString valgrindExecutable() const;
    void setValgrindExecutable(const QString &executable);
    QStringList valgrindArguments() const;
    void setValgrindArguments(const QStringList &toolArguments);
    QString debuggeeExecutable() const;
    void setDebuggeeExecutable(const QString &executable);
    QString debuggeeArguments() const;
    void setDebuggeeArguments(const QString &arguments);

    void setWorkingDirectory(const QString &path);
    QString workingDirectory() const;
    void setEnvironment(const Utils::Environment &environment);
    void setProcessChannelMode(QProcess::ProcessChannelMode mode);

    void waitForFinished() const;

    QString errorString() const;

    virtual void start();

    virtual void stop();

protected:
    virtual QString tool() const = 0;

    Q_PID lastPid() const;

signals:
    void standardOutputReceived(const QByteArray &);
    void standardErrorReceived(const QByteArray &);
    void processErrorReceived(const QString &, QProcess::ProcessError);
    void started();
    void finished();

protected slots:
    virtual void processError(QProcess::ProcessError);
    virtual void processStarted();
    virtual void processFinished(int, QProcess::ExitStatus);

private:
    Q_DISABLE_COPY(ValgrindRunner);
    class Private;
    Private *d;
};

}

#endif // VALGRIND_RUNNER_H
