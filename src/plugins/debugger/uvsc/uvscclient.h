/****************************************************************************
**
** Copyright (C) 2020 Denis Shienkov <denis.shienkov@gmail.com>
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

namespace Debugger {
namespace Internal {

// UvscClient

class UvscClient final : public QObject
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

    explicit UvscClient(const QDir &uvscDir);
    ~UvscClient() final;

    void version(QString &uvscVersion, QString &uvsockVersion);

    bool connectSession(qint32 uvscPort);
    void disconnectSession();
    bool startSession(bool extendedStack);
    bool stopSession();

    bool executeStepOver();
    bool executeStepIn();
    bool executeStepOut();
    bool executeStepInstruction();

    bool startExecution();
    bool stopExecution();

    bool fetchThreads(bool showNames, GdbMi &data);
    bool fetchStackFrames(quint32 taskId, quint64 address, GdbMi &data);
    bool fetchRegisters(Registers &registers);

    bool fetchLocals(const QStringList &expandedLocalINames, qint32 taskId,
                     qint32 frameId, GdbMi &data);

    bool fetchWatchers(const QStringList &expandedWatcherINames,
                       const std::vector<std::pair<QString, QString>> &rootWatchers, GdbMi &data);

    bool fetchMemory(quint64 address, QByteArray &data);
    bool changeMemory(quint64 address, const QByteArray &data);

    bool disassemblyAddress(quint64 address, QByteArray &result);

    bool setRegisterValue(int index, const QString &value);
    bool setLocalValue(qint32 localId, qint32 taskId, qint32 frameId, const QString &value);
    bool setWatcherValue(qint32 watcherId, const QString &value);

    bool createBreakpoint(const QString &exp, quint32 &tickMark, quint64 &address,
                          quint32 &line, QString &function, QString &fileName);
    bool deleteBreakpoint(quint32 tickMark);
    bool enableBreakpoint(quint32 tickMark);
    bool disableBreakpoint(quint32 tickMark);

    bool calculateExpression(const QString &exp, QByteArray &);

    bool openProject(const Utils::FilePath &projectFile);
    void closeProject();
    bool setProjectSources(const Utils::FilePath &sourceDirectory,
                           const Utils::FilePaths &sourceFiles);
    bool setProjectDebugTarget(bool simulator);
    bool setProjectOutputTarget(const Utils::FilePath &outputFile);

    // Helper functions (for testing purposes only).
    void showWindow();
    void hideWindow();

    UvscError error() const;
    QString errorString() const;

signals:
    void errorOccurred(UvscError error);
    void executionStarted();
    void executionStopped();
    void projectClosed();
    void locationUpdated(quint64 address);

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

} // namespace Internal
} // namespace Debugger
