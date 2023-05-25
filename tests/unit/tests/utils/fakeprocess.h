// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QProcess>

class FakeProcess : public QObject
{
    Q_OBJECT

public:
    FakeProcess();
    ~FakeProcess();

    void finishUnsuccessfully();
    void finishByCrash();
    void finish();

    void start();
    void setArguments(const QStringList &arguments);
    void setProgram(const QString &program);

    void setProcessChannelMode(QProcess::ProcessChannelMode mode);


    bool isStarted() const;

    const QStringList &arguments() const;
    const QString &applicationPath() const;

signals:
    void finished(int exitCode, QProcess::ExitStatus exitStatus);

private:
    QStringList m_arguments;
    QString m_applicationPath;
    bool m_isFinished = false;
    bool m_isStarted = false;
};
