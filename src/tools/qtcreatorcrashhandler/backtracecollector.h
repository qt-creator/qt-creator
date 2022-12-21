// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QProcess>

class BacktraceCollectorPrivate;

class BacktraceCollector : public QObject
{
    Q_OBJECT
public:
    explicit BacktraceCollector(QObject *parent = 0);
    ~BacktraceCollector();

    void run(qint64 pid);
    bool isRunning() const;
    void kill();

signals:
    void error(const QString &errorMessage);
    void backtrace(const QString &backtrace);
    void backtraceChunk(const QString &chunk);

private:
    void onDebuggerOutputAvailable();
    void onDebuggerFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onDebuggerError(QProcess::ProcessError err);

    QString createTemporaryCommandFile();

    BacktraceCollectorPrivate *d;
};
