/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef TCFTRKENGINE_H
#define TCFTRKENGINE_H

#include "symbianutils_global.h"
#include "tcftrkmessage.h"
#include "callback.h"
#include "json.h"

#include <QtCore/QObject>
#include <QtCore/QSharedPointer>
#include <QtCore/QVector>
#include <QtCore/QVariant>
#include <QtCore/QStringList>
#include <QtCore/QDateTime>

QT_BEGIN_NAMESPACE
class QIODevice;
class QTextStream;
QT_END_NAMESPACE

namespace tcftrk {

struct TcfTrkDevicePrivate;
struct Breakpoint;

/* Command error handling in TCF:
 * 1) 'Severe' errors (JSON format, parameter format): Trk emits a
 *     nonstandard message (\3\2 error parameters) and closes the connection.
 * 2) Protocol errors: 'N' without error message is returned.
 * 3) Errors in command execution: 'R' with a TCF error hash is returned
 *    (see TcfTrkCommandError). */

/* Error code return in 'R' reply to command
 * (see top of 'Services' documentation). */
struct SYMBIANUTILS_EXPORT TcfTrkCommandError {
    TcfTrkCommandError();
    void clear();
    bool isError() const;
    operator bool() const { return isError(); }
    QString toString() const;
    void write(QTextStream &str) const;
    bool parse(const QVector<JsonValue> &values);

    quint64 timeMS; // Since 1.1.1970
    qint64 code;
    QByteArray format; // message
    // 'Alternative' meaning, like altOrg="POSIX"/altCode=<some errno>
    QByteArray alternativeOrganization;
    qint64 alternativeCode;
};

/* Answer to a Tcf command passed to the callback. */
struct SYMBIANUTILS_EXPORT TcfTrkCommandResult {
    enum Type
    {
        SuccessReply,       // 'R' and no error -> all happy.
        CommandErrorReply,  // 'R' with TcfTrkCommandError received
        ProgressReply,      // 'P', progress indicator
        FailReply           // 'N' Protocol NAK, severe error
    };

    explicit TcfTrkCommandResult(Type t = SuccessReply);
    explicit TcfTrkCommandResult(char typeChar, Services service,
                                 const QByteArray &request,
                                 const QVector<JsonValue> &values,
                                 const QVariant &cookie);

    QString toString() const;
    QString errorString() const;
    operator bool() const { return type == SuccessReply || type == ProgressReply; }

    static QDateTime tcfTimeToQDateTime(quint64 tcfTimeMS);

    Type type;
    Services service;
    QByteArray request;
    TcfTrkCommandError commandError;
    QVector<JsonValue> values;
    QVariant cookie;
};

// Response to stat/fstat
struct SYMBIANUTILS_EXPORT TcfTrkStatResponse
{
    TcfTrkStatResponse();

    quint64 size;
    QDateTime modTime;
    QDateTime accessTime;
};

typedef trk::Callback<const TcfTrkCommandResult &> TcfTrkCallback;

/* TcfTrkDevice: TCF communication helper using an asynchronous QIODevice
 * implementing the TCF protocol according to:
http://dev.eclipse.org/svnroot/dsdp/org.eclipse.tm.tcf/trunk/docs/TCF%20Specification.html
http://dev.eclipse.org/svnroot/dsdp/org.eclipse.tm.tcf/trunk/docs/TCF%20Services.html
 * Commands can be sent along with callbacks that are passed a
 * TcfTrkCommandResult and an opaque QVariant cookie. In addition, events are emitted.
 *
 * Note: As of 11.8.2010, TCF Trk 4.0.5 does not currently support 'Registers::getm'
 * (get multiple registers). So, TcfTrkDevice emulates it by sending a sequence of
 * single commands. As soon as 'Registers::getm' is natively supported, all code
 * related to 'FakeRegisterGetm' should be removed. The workaround requires that
 * the register name is known.
 * CODA notes:
 * - Commands are accepted only after receiving the Locator Hello event
 * - Serial communication initiation sequence:
 *     Send serial ping from host sendSerialPing() -> receive pong response with
 *     version information -> Send Locator Hello Event -> Receive Locator Hello Event
 *     -> Commands are accepted.
 * - WLAN communication initiation sequence:
 *     Receive Locator Hello Event from CODA -> Commands are accepted.
 */

class SYMBIANUTILS_EXPORT TcfTrkDevice : public QObject
{
    Q_PROPERTY(unsigned verbose READ verbose WRITE setVerbose)
    Q_PROPERTY(bool serialFrame READ serialFrame WRITE setSerialFrame)
    Q_OBJECT
public:
    // Flags for FileSystem:open
    enum FileSystemOpenFlags
    {
        FileSystem_TCF_O_READ = 0x00000001,
        FileSystem_TCF_O_WRITE = 0x00000002,
        FileSystem_TCF_O_APPEND = 0x00000004,
        FileSystem_TCF_O_CREAT = 0x00000008,
        FileSystem_TCF_O_TRUNC = 0x00000010,
        FileSystem_TCF_O_EXCL = 0x00000020
    };

    enum MessageType
    {
        MessageWithReply,
        MessageWithoutReply, /* Non-standard: "Settings:set" command does not reply */
        NoopMessage
    };

    typedef QSharedPointer<QIODevice> IODevicePtr;

    explicit TcfTrkDevice(QObject *parent = 0);
    virtual ~TcfTrkDevice();

    unsigned verbose() const;
    bool serialFrame() const;
    void setSerialFrame(bool);

    // Mapping of register names to indices for multi-requests.
    // Register names can be retrieved via 'Registers:getChildren' (requires
    // context id to be stripped).
    QVector<QByteArray> registerNames() const;
    void setRegisterNames(const QVector<QByteArray>& n);

    IODevicePtr device() const;
    IODevicePtr takeDevice();
    void setDevice(const IODevicePtr &dp);

    // Serial Only: Initiate communication. Will emit serialPong() signal with version.
    void sendSerialPing(bool pingOnly = false);

    // Send with parameters from string (which may contain '\0').
    void sendTcfTrkMessage(MessageType mt, Services service,
                           const char *command,
                           const char *commandParameters, int commandParametersLength,
                           const TcfTrkCallback &callBack = TcfTrkCallback(),
                           const QVariant &cookie = QVariant());

    void sendTcfTrkMessage(MessageType mt, Services service, const char *command,
                           const QByteArray &commandParameters,
                           const TcfTrkCallback &callBack = TcfTrkCallback(),
                           const QVariant &cookie = QVariant());

    // Convenience messages: Start a process
    void sendProcessStartCommand(const TcfTrkCallback &callBack,
                                 const QString &binary,
                                 unsigned uid,
                                 QStringList arguments = QStringList(),
                                 QString workingDirectory = QString(),
                                 bool debugControl = true,
                                 const QStringList &additionalLibraries = QStringList(),
                                 const QVariant &cookie = QVariant());

    // Preferred over Processes:Terminate by TCF TRK.
    void sendRunControlTerminateCommand(const TcfTrkCallback &callBack,
                                        const QByteArray &id,
                                        const QVariant &cookie = QVariant());

    void sendProcessTerminateCommand(const TcfTrkCallback &callBack,
                                     const QByteArray &id,
                                     const QVariant &cookie = QVariant());

    // Non-standard: Remove executable from settings.
    // Probably needs to be called after stopping. This command has no response.
    void sendSettingsRemoveExecutableCommand(const QString &binaryIn,
                                             unsigned uid,
                                             const QStringList &additionalLibraries = QStringList(),
                                             const QVariant &cookie = QVariant());

    void sendRunControlSuspendCommand(const TcfTrkCallback &callBack,
                                      const QByteArray &id,
                                      const QVariant &cookie = QVariant());

    // Resume / Step (see RunControlResumeMode).
    void sendRunControlResumeCommand(const TcfTrkCallback &callBack,
                                     const QByteArray &id,
                                     RunControlResumeMode mode,
                                     unsigned count /* = 1, currently ignored. */,
                                     quint64 rangeStart, quint64 rangeEnd,
                                     const QVariant &cookie = QVariant());

    // Convenience to resume a suspended process
    void sendRunControlResumeCommand(const TcfTrkCallback &callBack,
                                     const QByteArray &id,
                                     const QVariant &cookie = QVariant());

    void sendBreakpointsAddCommand(const TcfTrkCallback &callBack,
                                   const Breakpoint &b,
                                   const QVariant &cookie = QVariant());

    void sendBreakpointsRemoveCommand(const TcfTrkCallback &callBack,
                                      const QByteArray &id,
                                      const QVariant &cookie = QVariant());

    void sendBreakpointsRemoveCommand(const TcfTrkCallback &callBack,
                                      const QVector<QByteArray> &id,
                                      const QVariant &cookie = QVariant());

    void sendBreakpointsEnableCommand(const TcfTrkCallback &callBack,
                                      const QByteArray &id,
                                      bool enable,
                                      const QVariant &cookie = QVariant());

    void sendBreakpointsEnableCommand(const TcfTrkCallback &callBack,
                                      const QVector<QByteArray> &id,
                                      bool enable,
                                      const QVariant &cookie = QVariant());


    void sendMemoryGetCommand(const TcfTrkCallback &callBack,
                              const QByteArray &contextId,
                              quint64 start, quint64 size,
                              const QVariant &cookie = QVariant());

    void sendMemorySetCommand(const TcfTrkCallback &callBack,
                              const QByteArray &contextId,
                              quint64 start, const QByteArray& data,
                              const QVariant &cookie = QVariant());

    // Get register names (children of context).
    // It is possible to recurse from  thread id down to single registers.
    void sendRegistersGetChildrenCommand(const TcfTrkCallback &callBack,
                                         const QByteArray &contextId,
                                         const QVariant &cookie = QVariant());

    // Register get
    void sendRegistersGetCommand(const TcfTrkCallback &callBack,
                                 const QByteArray &contextId,
                                 QByteArray id,
                                 const QVariant &cookie);

    void sendRegistersGetMCommand(const TcfTrkCallback &callBack,
                                  const QByteArray &contextId,
                                  const QVector<QByteArray> &ids,
                                  const QVariant &cookie = QVariant());

    // Convenience to get a range of register "R0" .. "R<n>".
    // Cookie will be an int containing "start".
    void sendRegistersGetMRangeCommand(const TcfTrkCallback &callBack,
                                 const QByteArray &contextId,
                                  unsigned start, unsigned count);

    // Set register
    void sendRegistersSetCommand(const TcfTrkCallback &callBack,
                                 const QByteArray &contextId,
                                 QByteArray ids,
                                 const QByteArray &value, // binary value
                                 const QVariant &cookie = QVariant());
    // Set register
    void sendRegistersSetCommand(const TcfTrkCallback &callBack,
                                 const QByteArray &contextId,
                                 unsigned registerNumber,
                                 const QByteArray &value, // binary value
                                 const QVariant &cookie = QVariant());

    // File System
    void sendFileSystemOpenCommand(const TcfTrkCallback &callBack,
                                   const QByteArray &name,
                                   unsigned flags = FileSystem_TCF_O_READ,
                                   const QVariant &cookie = QVariant());

    void sendFileSystemFstatCommand(const TcfTrkCallback &callBack,
                                   const QByteArray &handle,
                                   const QVariant &cookie = QVariant());

    void sendFileSystemWriteCommand(const TcfTrkCallback &callBack,
                                    const QByteArray &handle,
                                    const QByteArray &data,
                                    unsigned offset = 0,
                                    const QVariant &cookie = QVariant());

    void sendFileSystemCloseCommand(const TcfTrkCallback &callBack,
                                   const QByteArray &handle,
                                   const QVariant &cookie = QVariant());

    // Symbian Install
    void sendSymbianInstallSilentInstallCommand(const TcfTrkCallback &callBack,
                                                const QByteArray &file,
                                                const QByteArray &targetDrive,
                                                const QVariant &cookie = QVariant());

    void sendSymbianInstallUIInstallCommand(const TcfTrkCallback &callBack,
                                            const QByteArray &file,
                                            const QVariant &cookie = QVariant());

    void sendLoggingAddListenerCommand(const TcfTrkCallback &callBack,
                                       const QVariant &cookie = QVariant());

    static QByteArray parseMemoryGet(const TcfTrkCommandResult &r);
    static QVector<QByteArray> parseRegisterGetChildren(const TcfTrkCommandResult &r);
    static TcfTrkStatResponse parseStat(const TcfTrkCommandResult &r);

signals:
    void genericTcfEvent(int service, const QByteArray &name, const QVector<tcftrk::JsonValue> &value);
    void tcfEvent(const tcftrk::TcfTrkEvent &knownEvent);
    void serialPong(const QString &codaVersion);

    void logMessage(const QString &);
    void error(const QString &);

public slots:
    void setVerbose(unsigned v);

private slots:
    void slotDeviceError();
    void slotDeviceSocketStateChanged();
    void slotDeviceReadyRead();

private:
    void deviceReadyReadSerial();
    void deviceReadyReadTcp();

    bool checkOpen();
    void checkSendQueue();
    void writeMessage(QByteArray data, bool ensureTerminating0 = true);
    void emitLogMessage(const QString &);
    inline int parseMessage(const QByteArray &);
    void processMessage(const QByteArray &message);
    inline void processSerialMessage(const QByteArray &message);
    int parseTcfCommandReply(char type, const QVector<QByteArray> &tokens);
    int parseTcfEvent(const QVector<QByteArray> &tokens);
    // Send with parameters from string (which may contain '\0').
    void sendTcfTrkMessage(MessageType mt, Services service,
                           const char *command,
                           const char *commandParameters, int commandParametersLength,
                           const TcfTrkCallback &callBack, const QVariant &cookie,
                           unsigned specialHandling);

    TcfTrkDevicePrivate *d;
};

} // namespace tcftrk

#endif // TCFTRKENGINE_H
