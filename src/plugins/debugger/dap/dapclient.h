// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <debugger/debuggerengine.h>

#include <utils/process.h>

#include <QLoggingCategory>

namespace Debugger::Internal {

class IDataProvider : public QObject
{
    Q_OBJECT
public:
    IDataProvider(QObject *parent = nullptr) : QObject(parent) {}
    ~IDataProvider() override = default;
    virtual void start() = 0;
    virtual bool isRunning() const = 0;
    virtual void writeRaw(const QByteArray &input) = 0;
    virtual void kill() = 0;
    virtual QByteArray readAllStandardOutput() = 0;
    virtual QString readAllStandardError() = 0;
    virtual int exitCode() const = 0;
    virtual QString executable() const = 0;

    virtual QProcess::ExitStatus exitStatus() const = 0;
    virtual QProcess::ProcessError error() const = 0;
    virtual Utils::ProcessResult result() const = 0;
    virtual QString exitMessage() const = 0;

signals:
    void started();
    void done();
    void readyReadStandardOutput();
    void readyReadStandardError();
};

enum class DapResponseType
{
    Initialize,
    ConfigurationDone,
    Continue,
    StackTrace,
    Scopes,
    DapThreads,
    Variables,
    StepIn,
    StepOut,
    StepOver,
    Pause,
    Evaluate,
    Unknown
};

enum class DapEventType
{
    Initialized,
    Stopped,
    Exited,
    DapThread,
    Output,
    DapBreakpoint,
    Unknown
};

class DapClient : public QObject
{
    Q_OBJECT

public:
    DapClient(IDataProvider *dataProvider, QObject *parent = nullptr);
    ~DapClient() override;

    IDataProvider *dataProvider() const { return m_dataProvider; }

    void postRequest(const QString &command, const QJsonObject &arguments = {});

    virtual void sendInitialize();

    void sendLaunch(const Utils::CommandLine &command);
    void sendAttach();
    void sendConfigurationDone();

    void sendDisconnect();
    void sendTerminate();

    void sendPause();
    void sendContinue(int threadId);

    void sendStepIn(int threadId);
    void sendStepOut(int threadId);
    void sendStepOver(int threadId);

    void evaluateVariable(const QString &expression, int frameId);

    void stackTrace(int threadId);
    void scopes(int frameId);
    void threads();
    void variables(int variablesReference);
    void setBreakpoints(const QJsonArray &breakpoints, const Utils::FilePath &fileName);

    void emitSignals(const QJsonDocument &doc);

signals:
    void started();
    void done();
    void readyReadStandardError();

    void responseReady(DapResponseType type, const QJsonObject &response);
    void eventReady(DapEventType type, const QJsonObject &response);

private:
    void readOutput();

    virtual const QLoggingCategory &logCategory()
    {
        static const QLoggingCategory logCategory = QLoggingCategory("qtc.dbg.dapengine",
                                                                     QtWarningMsg);
        return logCategory;
    }

    IDataProvider *m_dataProvider = nullptr;
    QByteArray m_inbuffer;
};

} // namespace Debugger::Internal
