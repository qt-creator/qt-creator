// Copyright (C) 2020 Denis Shienkov <denis.shienkov@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <debugger/debuggerprotocol.h>
#include <debugger/registerhandler.h>

#include <QObject>

QT_BEGIN_NAMESPACE
class QDir;
QT_END_NAMESPACE

// From UVSC api.
struct STACKENUM;
struct BKRSP;

namespace Utils { class FilePath; }

namespace Debugger::Internal {

// What UvscImpl needs from the uVision socket, so that tests can stand in for uVision.
class DEBUGGER_EXPORT UvscClientInterface : public QObject
{
    Q_OBJECT

public:
    using Registers = std::map<int, Register>;

    enum UvscError {
        NoError,
        InitializationError,
        ConnectionError,
        ConfigurationError,
        RuntimeError,
    };

    virtual void version(QString &uvscVersion, QString &uvsockVersion) = 0;

    virtual bool connectSession(qint32 uvscPort) = 0;
    virtual void disconnectSession() = 0;
    virtual bool startSession(bool extendedStack) = 0;
    virtual bool stopSession() = 0;

    virtual bool executeStepOver() = 0;
    virtual bool executeStepIn() = 0;
    virtual bool executeStepOut() = 0;
    virtual bool executeStepInstruction() = 0;

    virtual bool startExecution() = 0;
    virtual bool stopExecution() = 0;

    virtual bool fetchThreads(bool showNames, GdbMi &data) = 0;
    virtual bool fetchStackFrames(quint32 taskId, quint64 address, GdbMi &data) = 0;
    virtual bool fetchRegisters(Registers &registers) = 0;

    virtual bool fetchLocals(const QStringList &expandedLocalINames, qint32 taskId,
                             qint32 frameId, GdbMi &data) = 0;

    virtual bool fetchWatchers(const QStringList &expandedWatcherINames,
                               const std::vector<std::pair<QString, QString>> &rootWatchers,
                               GdbMi &data) = 0;

    virtual bool fetchMemory(quint64 address, QByteArray &data) = 0;
    virtual bool changeMemory(quint64 address, const QByteArray &data) = 0;

    virtual bool disassemblyAddress(quint64 address, QByteArray &result) = 0;

    virtual bool setRegisterValue(int index, const QString &value) = 0;
    virtual bool setLocalValue(qint32 localId, qint32 taskId, qint32 frameId,
                               const QString &value) = 0;
    virtual bool setWatcherValue(qint32 watcherId, const QString &value) = 0;

    virtual bool createBreakpoint(const QString &exp, quint32 &tickMark, quint64 &address,
                                  quint32 &line, QString &function, QString &fileName) = 0;
    virtual bool deleteBreakpoint(quint32 tickMark) = 0;
    virtual bool enableBreakpoint(quint32 tickMark) = 0;
    virtual bool disableBreakpoint(quint32 tickMark) = 0;

    virtual bool openProject(const Utils::FilePath &projectFile) = 0;
    virtual void closeProject() = 0;
    virtual bool setProjectDebugTarget(bool simulator) = 0;
    virtual bool setProjectOutputTarget(const Utils::FilePath &outputFile) = 0;

    virtual UvscError error() const = 0;
    virtual QString errorString() const = 0;

signals:
    void errorOccurred(UvscError error);
    void executionStarted();
    void executionStopped();
    void projectClosed();
    void locationUpdated(quint64 address);
};

// UvscClient

class UvscClient final : public UvscClientInterface
{
public:
    explicit UvscClient(const QDir &uvscDir);
    ~UvscClient() final;

    void version(QString &uvscVersion, QString &uvsockVersion) final;

    bool connectSession(qint32 uvscPort) final;
    void disconnectSession() final;
    bool startSession(bool extendedStack) final;
    bool stopSession() final;

    bool executeStepOver() final;
    bool executeStepIn() final;
    bool executeStepOut() final;
    bool executeStepInstruction() final;

    bool startExecution() final;
    bool stopExecution() final;

    bool fetchThreads(bool showNames, GdbMi &data) final;
    bool fetchStackFrames(quint32 taskId, quint64 address, GdbMi &data) final;
    bool fetchRegisters(Registers &registers) final;

    bool fetchLocals(const QStringList &expandedLocalINames, qint32 taskId,
                     qint32 frameId, GdbMi &data) final;

    bool fetchWatchers(const QStringList &expandedWatcherINames,
                       const std::vector<std::pair<QString, QString>> &rootWatchers,
                       GdbMi &data) final;

    bool fetchMemory(quint64 address, QByteArray &data) final;
    bool changeMemory(quint64 address, const QByteArray &data) final;

    bool disassemblyAddress(quint64 address, QByteArray &result) final;

    bool setRegisterValue(int index, const QString &value) final;
    bool setLocalValue(qint32 localId, qint32 taskId, qint32 frameId,
                       const QString &value) final;
    bool setWatcherValue(qint32 watcherId, const QString &value) final;

    bool createBreakpoint(const QString &exp, quint32 &tickMark, quint64 &address,
                          quint32 &line, QString &function, QString &fileName) final;
    bool deleteBreakpoint(quint32 tickMark) final;
    bool enableBreakpoint(quint32 tickMark) final;
    bool disableBreakpoint(quint32 tickMark) final;

    bool calculateExpression(const QString &exp, QByteArray &);

    bool openProject(const Utils::FilePath &projectFile) final;
    void closeProject() final;
    bool setProjectSources(const Utils::FilePath &sourceDirectory,
                           const Utils::FilePaths &sourceFiles);
    bool setProjectDebugTarget(bool simulator) final;
    bool setProjectOutputTarget(const Utils::FilePath &outputFile) final;

    // Helper functions (for testing purposes only).
    void showWindow();
    void hideWindow();

    UvscError error() const final;
    QString errorString() const final;

private:
    void customEvent(QEvent *event) final;

    bool checkConnection();
    bool enumerateStack(quint32 taskId, std::vector<STACKENUM> &stackenums);
    bool inspectLocal(const QStringList &expandedLocalINames, const QString &localIName,
                      qint32 localId, qint32 taskId, qint32 frameId, GdbMi &data);
    bool fetchWatcher(const QStringList &expandedWatcherINames,
                      const std::pair<QString, QString> &rootWatcher, GdbMi &data);
    bool inspectWatcher(const QStringList &expandedWatcherINames, qint32 watcherId,
                        const QString &watcherIName, GdbMi &parent);
    void setError(UvscError error, const QString &errorString = QString());
    void handleMsgEvent(QEvent *event);
    void updateLocation(const QByteArray &bpreason);
    bool addressToFileLine(quint64 address, QString &fileName, QString &function, quint32 &line);

    bool controlHiddenBreakpoint(const QString &exp);
    bool enumerateBreakpoints(std::vector<BKRSP> &bpenums);
    bool executeCommand(const QString &cmd, QString &output);

    qint32 m_descriptor = -1;
    quint64 m_exitAddress = 0;
    UvscError m_error = NoError;
    QString m_errorString;
};

} // Debugger::Internal
