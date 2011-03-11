/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Author: Milian Wolff, KDAB (milian.wolff@kdab.com)
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "valgrindprocess.h"

#include <QtCore/QDebug>
#include <QtCore/QEventLoop>
#include <QtCore/QFileInfo>

namespace Valgrind {
namespace Internal {

ValgrindProcess::ValgrindProcess(QObject *parent)
: QObject(parent)
{

}

ValgrindProcess::~ValgrindProcess()
{

}

////////////////////////

LocalValgrindProcess::LocalValgrindProcess(QObject *parent)
: ValgrindProcess(parent)
{
    connect(&m_process, SIGNAL(finished(int, QProcess::ExitStatus)),
            this, SIGNAL(finished(int, QProcess::ExitStatus)));
    connect(&m_process, SIGNAL(started()),
            this, SIGNAL(started()));
    connect(&m_process, SIGNAL(error(QProcess::ProcessError)),
            this, SIGNAL(error(QProcess::ProcessError)));
    connect(&m_process, SIGNAL(readyReadStandardError()),
            this, SLOT(readyReadStandardError()));
    connect(&m_process, SIGNAL(readyReadStandardOutput()),
            this, SLOT(readyReadStandardOutput()));
}

LocalValgrindProcess::~LocalValgrindProcess()
{

}

void LocalValgrindProcess::setProcessChannelMode(QProcess::ProcessChannelMode mode)
{
    m_process.setProcessChannelMode(mode);
}

void LocalValgrindProcess::setWorkingDirectory(const QString &path)
{
    m_process.setWorkingDirectory(path);
}

bool LocalValgrindProcess::isRunning() const
{
    return m_process.state() != QProcess::NotRunning;
}

void LocalValgrindProcess::setEnvironment(const Utils::Environment &environment)
{
    m_process.setEnvironment(environment);
}

void LocalValgrindProcess::close()
{
    m_process.close();
    m_process.waitForFinished(-1);
}

void LocalValgrindProcess::run(const QString &valgrindExecutable, const QStringList &valgrindArguments,
                                const QString &debuggeeExecutable, const QString &debuggeeArguments)
{
    QString arguments;
    Utils::QtcProcess::addArgs(&arguments, valgrindArguments);
#ifdef Q_OS_MAC
    // May be slower to start but without it we get no filenames for symbols.
    Utils::QtcProcess::addArg(&arguments, QLatin1String("--dsymutil=yes"));
#endif

    Utils::QtcProcess::addArg(&arguments, debuggeeExecutable);
    Utils::QtcProcess::addArgs(&arguments, debuggeeArguments);

    m_process.setCommand(valgrindExecutable, arguments);
    m_process.start();
    m_process.waitForStarted();
}

QString LocalValgrindProcess::errorString() const
{
    return m_process.errorString();
}

QProcess::ProcessError LocalValgrindProcess::error() const
{
    return m_process.error();
}

Q_PID LocalValgrindProcess::pid() const
{
    return m_process.pid();
}

void LocalValgrindProcess::readyReadStandardError()
{
    const QByteArray b = m_process.readAllStandardError();
    if (!b.isEmpty())
        emit standardErrorReceived(b);
}

void LocalValgrindProcess::readyReadStandardOutput()
{
    const QByteArray b = m_process.readAllStandardOutput();
    if (!b.isEmpty())
        emit standardOutputReceived(b);
}

}
}
