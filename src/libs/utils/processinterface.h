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
    ProcessInterface(QObject *parent = nullptr) : QObject(parent) {}

signals:
    // This should be emitted when being in Starting state only.
    // After emitting this signal the process enters Running state.
    void started(qint64 processId, qint64 applicationMainThreadId = 0);

    // This should be emitted when being in Running state only.
    void readyRead(const QByteArray &outputData, const QByteArray &errorData);

    // This should be emitted when being in Starting or Running state.
    // When being in Starting state, the resultData should set error to FailedToStart.
    // After emitting this signal the process enters NotRunning state.
    void done(const Utils::ProcessResultData &resultData);

protected:
    ProcessSetupData m_setup;

private:
    // It's being called only in Starting state. Just before this method is being called,
    // the process transitions from NotRunning into Starting state.
    virtual void start() = 0;

    // It's being called only in Running state.
    virtual qint64 write(const QByteArray &data) = 0;

    // It's being called in Starting or Running state.
    virtual void sendControlSignal(ControlSignal controlSignal) = 0;

    // It's being called only in Starting state.
    virtual bool waitForStarted(int msecs) = 0;

    // It's being called in Starting or Running state.
    virtual bool waitForReadyRead(int msecs) = 0;

    // It's being called in Starting or Running state.
    virtual bool waitForFinished(int msecs) = 0;

    friend class QtcProcess;
};

} // namespace Utils
