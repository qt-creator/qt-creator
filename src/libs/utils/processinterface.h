/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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

#pragma once

#include "utils_global.h"

#include "environment.h"
#include "commandline.h"
#include "processenums.h"

#include <QProcess>

namespace Utils {

class QTCREATOR_UTILS_EXPORT ProcessSetupData
{
public:
    using Ptr = std::shared_ptr<ProcessSetupData>;

    ProcessImpl m_processImpl = ProcessImpl::Default;
    ProcessMode m_processMode = ProcessMode::Reader;
    TerminalMode m_terminalMode = TerminalMode::Off;

    CommandLine m_commandLine;
    FilePath m_workingDirectory;
    Environment m_environment;
    Environment m_remoteEnvironment;
    QByteArray m_writeData;
    QVariantHash m_extraData;
    QString m_standardInputFile;
    QString m_nativeArguments; // internal, dependent on specific code path

    bool m_abortOnMetaChars = true;
    bool m_runAsRoot = false;
    bool m_haveEnv = false;
    bool m_lowPriority = false;
    bool m_unixTerminalDisabled = false;
    bool m_useCtrlCStub = false; // release only
    bool m_belowNormalPriority = false; // internal, dependent on other fields and specific code path
};

class QTCREATOR_UTILS_EXPORT ProcessResultData
{
public:
    int m_exitCode = 0;
    QProcess::ExitStatus m_exitStatus = QProcess::NormalExit;
    QProcess::ProcessError m_error = QProcess::UnknownError;
    QString m_errorString;
};

enum class ControlSignal {
    Terminate,
    Kill,
    Interrupt,
    KickOff
};

class QTCREATOR_UTILS_EXPORT ProcessInterface : public QObject
{
    Q_OBJECT

public:
    ProcessInterface(QObject *parent = nullptr) : QObject(parent), m_setup(new ProcessSetupData) {}
    ProcessInterface(ProcessSetupData::Ptr setup) : m_setup(setup) {}

    virtual void start() = 0;
    virtual qint64 write(const QByteArray &data) = 0;
    virtual void sendControlSignal(ControlSignal controlSignal) = 0;

    virtual QProcess::ProcessState state() const = 0;

    virtual bool waitForStarted(int msecs) = 0;
    virtual bool waitForReadyRead(int msecs) = 0;
    virtual bool waitForFinished(int msecs) = 0;

signals:
    void started(qint64 processId, qint64 applicationMainThreadId = 0);
    void readyRead(const QByteArray &outputData, const QByteArray &errorData);
    void done(const Utils::ProcessResultData &resultData);

protected:
    ProcessSetupData::Ptr m_setup;
    friend class ProcessProxyInterface;
    friend class QtcProcess;
};

class QTCREATOR_UTILS_EXPORT ProcessProxyInterface : public ProcessInterface
{
    Q_OBJECT

public:
    ProcessProxyInterface(ProcessInterface *target)
        : ProcessInterface(target->m_setup)
        , m_target(target)
    {
        m_target->setParent(this);
        connect(m_target, &ProcessInterface::started, this, &ProcessInterface::started);
        connect(m_target, &ProcessInterface::readyRead, this, &ProcessInterface::readyRead);
        connect(m_target, &ProcessInterface::done, this, &ProcessInterface::done);
    }

    void start() override { m_target->start(); }

    qint64 write(const QByteArray &data) override { return m_target->write(data); }

    QProcess::ProcessState state() const override { return m_target->state(); }

    bool waitForStarted(int msecs) override { return m_target->waitForStarted(msecs); }
    bool waitForReadyRead(int msecs) override { return m_target->waitForReadyRead(msecs); }
    bool waitForFinished(int msecs) override { return m_target->waitForFinished(msecs); }

protected:
    ProcessInterface *m_target;
};


} // namespace Utils
