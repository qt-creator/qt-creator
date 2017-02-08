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

#include "backtracecollector.h"

#include <QDebug>
#include <QScopedPointer>
#include <QTemporaryFile>

const char GdbBatchCommands[] =
        "set height 0\n"
        "set width 0\n"
        "thread\n"
        "thread apply all backtrace full\n";

class BacktraceCollectorPrivate
{
public:
    BacktraceCollectorPrivate() : errorOccurred(false) {}

    bool errorOccurred;
    QScopedPointer<QTemporaryFile> commandFile;
    QProcess debugger;
    QString output;
};

BacktraceCollector::BacktraceCollector(QObject *parent) :
    QObject(parent), d(new BacktraceCollectorPrivate)
{
    connect(&d->debugger,
            static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
            this, &BacktraceCollector::onDebuggerFinished);
    connect(&d->debugger, &QProcess::errorOccurred, this, &BacktraceCollector::onDebuggerError);
    connect(&d->debugger, &QIODevice::readyRead,
            this, &BacktraceCollector::onDebuggerOutputAvailable);
    d->debugger.setProcessChannelMode(QProcess::MergedChannels);
}

BacktraceCollector::~BacktraceCollector()
{
    delete d;
}

void BacktraceCollector::run(Q_PID pid)
{
    d->debugger.start(QLatin1String("gdb"), QStringList({
        "--nw",    // Do not use a window interface.
        "--nx",    // Do not read .gdbinit file.
        "--batch", // Exit after processing options.
        "--command", createTemporaryCommandFile(), "--pid", QString::number(pid) })
    );
}

bool BacktraceCollector::isRunning() const
{
    return d->debugger.state() == QProcess::Running;
}

void BacktraceCollector::kill()
{
    d->debugger.kill();
}

void BacktraceCollector::onDebuggerFinished(int exitCode, QProcess::ExitStatus /*exitStatus*/)
{
    if (d->errorOccurred) {
        emit error(QLatin1String("QProcess: ") + d->debugger.errorString());
        return;
    }
    if (exitCode != 0) {
        emit error(QString::fromLatin1("Debugger exited with code %1.").arg(exitCode));
        return;
    }
    emit backtrace(d->output);
}

void BacktraceCollector::onDebuggerError(QProcess::ProcessError /*error*/)
{
    d->errorOccurred = true;
}

QString BacktraceCollector::createTemporaryCommandFile()
{
    d->commandFile.reset(new QTemporaryFile);
    if (!d->commandFile->open()) {
        emit error(QLatin1String("Error: Could not create temporary command file."));
        return QString();
    }
    if (d->commandFile->write(GdbBatchCommands) == -1) {
        emit error(QLatin1String("Error: Could not write temporary command file."));
        return QString();
    }
    d->commandFile->close();
    return d->commandFile->fileName();
}

void BacktraceCollector::onDebuggerOutputAvailable()
{
    const QString newChunk = QString::fromLocal8Bit(d->debugger.readAll());
    d->output.append(newChunk);
    emit backtraceChunk(newChunk);
}
