// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/qtcprocess.h>

#include <QObject>

namespace Squish::Internal {

enum SquishProcessState { Idle, Starting, Started, StartFailed, Stopped, StopFailed };

class SquishProcessBase : public QObject
{
    Q_OBJECT
public:
    explicit SquishProcessBase(QObject *parent = nullptr);
    ~SquishProcessBase() = default;
    SquishProcessState processState() const { return m_state; }

    bool isRunning() const { return m_process.isRunning(); }
    Utils::ProcessResult result() const { return m_process.result(); }
    QProcess::ProcessError error() const { return m_process.error(); }
    QProcess::ProcessState state() const { return m_process.state(); }

    void closeProcess() { m_process.close(); }

signals:
    void logOutputReceived(const QString &output);
    void stateChanged(SquishProcessState state);

protected:
    void setState(SquishProcessState state);
    virtual void onDone() {}
    virtual void onErrorOutput() {}

    Utils::QtcProcess m_process;

private:
    SquishProcessState m_state = Idle;
};

} // namespace Squish::Internal
