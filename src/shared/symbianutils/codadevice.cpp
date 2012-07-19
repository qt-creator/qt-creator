/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#include "codadevice.h"
#include "json.h"
#include "codautils.h"

#include <QAbstractSocket>
#include <QDebug>
#include <QVector>
#include <QQueue>
#include <QTextStream>
#include <QDateTime>
#include <QFileInfo>

enum { debug = 0 };

static const char tcpMessageTerminatorC[] = "\003\001";

// Serial Ping: 0xfc,0x1f
static const char serialPingC[] = "\xfc\x1f";
// Serial Pong: 0xfc,0xf1, followed by version info
static const char serialPongC[] = "\xfc\xf1";

static const char locatorAnswerC[] = "E\0Locator\0Hello\0[\"Locator\"]";

/* Serial messages > (1K - 2) have to chunked in order to pass the USB
 * router as '0xfe char(chunkCount - 1) data' ... '0x0 char(chunkCount - 2) data'
 * ... '0x0 0x0 last-data' */
static const unsigned serialChunkLength = 0x400;  // 1K max USB router
static const int maxSerialMessageLength = 0x10000; // given chunking scheme

static const char validProtocolIdStart = (char)0x90;
static const char validProtocolIdEnd = (char)0x95;
static const char codaProtocolId = (char)0x92;
static const unsigned char serialChunkingStart = 0xfe;
static const unsigned char serialChunkingContinuation = 0x0;
enum { SerialChunkHeaderSize = 2 };

// Create USB router frame
static inline void encodeSerialFrame(const QByteArray &data, QByteArray *target, char protocolId)
{
    target->append(char(0x01));
    target->append(protocolId);
    appendShort(target, ushort(data.size()), Coda::BigEndian);
    target->append(data);
}

// Split in chunks of 1K according to CODA protocol chunking
static inline QByteArray encodeUsbSerialMessage(const QByteArray &dataIn)
{
    // Reserve 2 header bytes
    static const int chunkSize = serialChunkLength - SerialChunkHeaderSize;
    const int size = dataIn.size();
    QByteArray frame;
    // Do we need to split?
    if (size < chunkSize) {  // Nope, all happy.
        frame.reserve(size + 4);
        encodeSerialFrame(dataIn, &frame, codaProtocolId);
        return frame;
    }
    // Split.
    unsigned chunkCount = size / chunkSize;
    if (size % chunkSize)
        chunkCount++;
    if (debug)
        qDebug("Serial: Splitting message of %d bytes into %u chunks of %d", size, chunkCount, chunkSize);

    frame.reserve((4 + serialChunkLength) * chunkCount);
    int pos = 0;
    for (unsigned c = chunkCount - 1; pos < size ; c--) {
        QByteArray chunk; // chunk with long message start/continuation code
        chunk.reserve(serialChunkLength);
        chunk.append(pos ? serialChunkingContinuation : serialChunkingStart);
        chunk.append(char(static_cast<unsigned char>(c))); // Avoid any signedness issues.
        const int chunkEnd = qMin(pos + chunkSize, size);
        chunk.append(dataIn.mid(pos, chunkEnd - pos));
        encodeSerialFrame(chunk, &frame, codaProtocolId);
        pos = chunkEnd;
    }
    if (debug > 1)
        qDebug("Serial chunked:\n%s", qPrintable(Coda::formatData(frame)));
    return frame;
}

using namespace Json;
namespace Coda {
// ------------- CodaCommandError

CodaCommandError::CodaCommandError() : timeMS(0), code(0), alternativeCode(0)
{
}

void CodaCommandError::clear()
{
    timeMS = 0;
    code = alternativeCode = 0;
    format.clear();
    alternativeOrganization.clear();
}

QDateTime CodaCommandResult::codaTimeToQDateTime(quint64 codaTimeMS)
{
    const QDateTime time(QDate(1970, 1, 1));
    return time.addMSecs(codaTimeMS);
}

void CodaCommandError::write(QTextStream &str) const
{
    if (isError()) {
        if (debug && timeMS)
            str << CodaCommandResult::codaTimeToQDateTime(timeMS).toString(Qt::ISODate) << ": ";
        str << "'" << format << '\'' //for symbian the format is the real error message
                << " Code: " << code;
        if (!alternativeOrganization.isEmpty())
            str << " ('" << alternativeOrganization << "', code: " << alternativeCode << ')';
    } else{
        str << "<No error>";
    }
}

QString CodaCommandError::toString() const
{
    QString rc;
    QTextStream str(&rc);
    write(str);
    return rc;
}

bool CodaCommandError::isError() const
{
    return timeMS != 0 || code != 0 || !format.isEmpty() || alternativeCode != 0;
}

/* {"Time":1277459762255,"Code":1,"AltCode":-6,"AltOrg":"POSIX","Format":"Unknown error: -6"} */
bool CodaCommandError::parse(const QVector<JsonValue> &values)
{
    // Parse an arbitrary hash (that could as well be a command response)
    // and check for error elements. It looks like sometimes errors are appended
    // to other values.
    unsigned errorKeyCount = 0;
    clear();
    do {
        if (values.isEmpty())
            break;
        // Errors are mostly appended, except for FileSystem::open, in which case
        // a string "null" file handle (sic!) follows the error.
        const int last = values.size() - 1;
        const int checkIndex = last == 1 && values.at(last).data() == "null" ?
                    last - 1 : last;
        if (values.at(checkIndex).type() != JsonValue::Object)
            break;
        foreach (const JsonValue &c, values.at(checkIndex).children()) {
            if (c.name() == "Time") {
                timeMS = c.data().toULongLong();
                errorKeyCount++;
            } else if (c.name() == "Code") {
                code = c.data().toLongLong();
                errorKeyCount++;
            } else if (c.name() == "Format") {
                format = c.data();
                errorKeyCount++;
            } else if (c.name() == "AltCode") {
                alternativeCode = c.data().toULongLong();
                errorKeyCount++;
            } else if (c.name() == "AltOrg") {
                alternativeOrganization = c.data();
                errorKeyCount++;
            }
        }
    } while (false);
    const bool errorFound = errorKeyCount >= 2u; // Should be at least 'Time', 'Code'.
    if (!errorFound)
        clear();
    if (debug) {
        qDebug("CodaCommandError::parse: Found error %d (%u): ", errorFound, errorKeyCount);
        if (!values.isEmpty())
            qDebug() << values.back().toString();
    }
    return errorFound;
}

// ------------ CodaCommandResult

CodaCommandResult::CodaCommandResult(Type t) :
    type(t), service(LocatorService)
{
}

CodaCommandResult::CodaCommandResult(char typeChar, Services s,
                                         const QByteArray &r,
                                         const QVector<JsonValue> &v,
                                         const QVariant &ck) :
    type(FailReply), service(s), request(r), values(v), cookie(ck)
{
    switch (typeChar) {
    case 'N':
        type = FailReply;
        break;
    case 'P':
        type = ProgressReply;
        break;
    case 'R':
        type = commandError.parse(values) ? CommandErrorReply : SuccessReply;
        break;
    default:
        qWarning("Unknown CODA's reply type '%c'", typeChar);
    }
}

QString CodaCommandResult::errorString() const
{
    QString rc;
    QTextStream str(&rc);

    switch (type) {
    case SuccessReply:
    case ProgressReply:
        str << "<No error>";
        return rc;
    case FailReply:
        str << "NAK";
        return rc;
    case CommandErrorReply:
        commandError.write(str);
        break;
    }
    if (debug) {
        // Append the failed command for reference
        str << " (Command was: '";
        QByteArray printableRequest = request;
        printableRequest.replace('\0', '|');
        str << printableRequest << "')";
    }
    return rc;
}

QString CodaCommandResult::toString() const
{
    QString rc;
    QTextStream str(&rc);
    str << "Command answer ";
    switch (type) {
    case SuccessReply:
        str << "[success]";
        break;
    case CommandErrorReply:
        str << "[command error]";
        break;
    case FailReply:
        str << "[fail (NAK)]";
        break;
    case ProgressReply:
        str << "[progress]";
        break;
    }
    str << ", " << values.size() << " values(s) to request: '";
    QByteArray printableRequest = request;
    printableRequest.replace('\0', '|');
    str << printableRequest << "' ";
    if (cookie.isValid())
        str << " cookie: " << cookie.toString();
    str << '\n';
    for (int i = 0, count = values.size(); i < count; i++)
        str << '#' << i << ' ' << values.at(i).toString() << '\n';
    if (type == CommandErrorReply)
        str << "Error: " << errorString();
    return rc;
}

CodaStatResponse::CodaStatResponse() : size(0)
{
}

struct CodaSendQueueEntry
{
    typedef CodaDevice::MessageType MessageType;

    explicit CodaSendQueueEntry(MessageType mt,
                                  int tok,
                                  Services s,
                                  const QByteArray &d,
                                  const CodaCallback &cb= CodaCallback(),
                                  const QVariant &ck = QVariant()) :
        messageType(mt), service(s), data(d), token(tok), cookie(ck), callback(cb) {}

    MessageType messageType;
    Services service;
    QByteArray data;
    int token;
    QVariant cookie;
    CodaCallback callback;
    unsigned specialHandling;
};

struct CodaDevicePrivate {
    typedef CodaDevice::IODevicePtr IODevicePtr;
    typedef QHash<int, CodaSendQueueEntry> TokenWrittenMessageMap;

    CodaDevicePrivate();

    const QByteArray m_tcpMessageTerminator;

    IODevicePtr m_device;
    unsigned m_verbose;
    QByteArray m_readBuffer;
    QByteArray m_serialBuffer; // for chunked messages
    int m_token;
    QQueue<CodaSendQueueEntry> m_sendQueue;
    TokenWrittenMessageMap m_writtenMessages;
    QVector<QByteArray> m_registerNames;
    QVector<QByteArray> m_fakeGetMRegisterValues;
    bool m_serialFrame;
    bool m_serialPingOnly;
};

CodaDevicePrivate::CodaDevicePrivate() :
    m_tcpMessageTerminator(tcpMessageTerminatorC),
    m_verbose(0), m_token(0), m_serialFrame(false), m_serialPingOnly(false)
{
}

CodaDevice::CodaDevice(QObject *parent) :
    QObject(parent), d(new CodaDevicePrivate)
{
    if (debug) setVerbose(true);
}

CodaDevice::~CodaDevice()
{
    delete d;
}

QVector<QByteArray> CodaDevice::registerNames() const
{
    return d->m_registerNames;
}

void CodaDevice::setRegisterNames(const QVector<QByteArray>& n)
{
    d->m_registerNames = n;
    if (d->m_verbose) {
        QString msg;
        QTextStream str(&msg);
        const int count = n.size();
        str << "Registers (" << count << "): ";
        for (int i = 0; i < count; i++)
            str << '#' << i << '=' << n.at(i) << ' ';
        emitLogMessage(msg);
    }
}

CodaDevice::IODevicePtr CodaDevice::device() const
{
    return d->m_device;
}

CodaDevice::IODevicePtr CodaDevice::takeDevice()
{
    const IODevicePtr old = d->m_device;
    if (!old.isNull()) {
        old.data()->disconnect(this);
        d->m_device = IODevicePtr();
    }
    d->m_readBuffer.clear();
    d->m_token = 0;
    d->m_sendQueue.clear();
    return old;
}

void CodaDevice::setDevice(const IODevicePtr &dp)
{
    if (dp.data() == d->m_device.data())
        return;
    if (dp.isNull()) {
        emitLogMessage(QLatin1String("Internal error: Attempt to set NULL device."));
        return;
    }
    takeDevice();
    d->m_device = dp;
    connect(dp.data(), SIGNAL(readyRead()), this, SLOT(slotDeviceReadyRead()));
    if (QAbstractSocket *s = qobject_cast<QAbstractSocket *>(dp.data())) {
        connect(s, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(slotDeviceError()));
        connect(s, SIGNAL(stateChanged(QAbstractSocket::SocketState)), this, SLOT(slotDeviceSocketStateChanged()));
    }
}

void CodaDevice::slotDeviceError()
{
    const QString message = d->m_device->errorString();
    emitLogMessage(message);
    emit error(message);
}

void CodaDevice::slotDeviceSocketStateChanged()
{
    if (const QAbstractSocket *s = qobject_cast<const QAbstractSocket *>(d->m_device.data())) {
        const QAbstractSocket::SocketState st = s->state();
        switch (st) {
        case QAbstractSocket::UnconnectedState:
            emitLogMessage(QLatin1String("Unconnected"));
            break;
        case QAbstractSocket::HostLookupState:
            emitLogMessage(QLatin1String("HostLookupState"));
            break;
        case QAbstractSocket::ConnectingState:
            emitLogMessage(QLatin1String("Connecting"));
            break;
        case QAbstractSocket::ConnectedState:
            emitLogMessage(QLatin1String("Connected"));
            break;
        case QAbstractSocket::ClosingState:
            emitLogMessage(QLatin1String("Closing"));
            break;
        default:
            emitLogMessage(QString::fromLatin1("State %1").arg(st));
            break;
        }
    }
}

static inline QString debugMessage(QByteArray  message, const char *prefix = 0)
{
    const bool isBinary = !message.isEmpty() && message.at(0) < 0;
    if (isBinary) {
        message = message.toHex(); // Some serial special message
    } else {
        message.replace('\0', '|');
    }
    const QString messageS = QString::fromLatin1(message);
    return prefix ?
            (QLatin1String(prefix) + messageS) :  messageS;
}

void CodaDevice::slotDeviceReadyRead()
{
    const QByteArray newData = d->m_device->readAll();
    d->m_readBuffer += newData;
    if (debug)
        qDebug("ReadBuffer: %s", qPrintable(Coda::stringFromArray(newData)));
    if (d->m_serialFrame) {
        deviceReadyReadSerial();
    } else {
        deviceReadyReadTcp();
    }
}

// Find a serial header in input stream '0x1', '0x92', 'lenH', 'lenL'
// and return message position and size.
QPair<int, int> CodaDevice::findSerialHeader(QByteArray &in)
{
    static const char header1 = 0x1;
   // Header should in theory always be at beginning of
    // buffer. Warn if there are bogus data in-between.

    while (in.size() >= 4) {
        if (in.at(0) == header1 && in.at(1) == codaProtocolId) {
            // Good packet
            const int length = Coda::extractShort(in.constData() + 2);
            return QPair<int, int>(4, length);
        } else if (in.at(0) == header1 && in.at(1) >= validProtocolIdStart && in.at(1) <= validProtocolIdEnd) {
            // We recognise it but it's not a CODA message - emit it for any interested party to handle
            const int length = Coda::extractShort(in.constData() + 2);
            if (4 + length <= in.size()) {
                // We have all the data
                QByteArray data(in.mid(4, length));
                emit unknownEvent(in.at(1), data);
                in.remove(0, 4+length);
                // and continue
            } else {
                // If we don't have all this packet, there can't be any data following it, so return now
                // and wait for more data
                return QPair<int, int>(-1, -1);
            }
        } else {
            // Bad data - log it, remove it, and go round again
            int nextHeader = in.indexOf(header1, 1);
            QByteArray bad = in.mid(0, nextHeader);
            qWarning("Bogus data received on serial line: %s\n"
                     "Frame Header at: %d", qPrintable(Coda::stringFromArray(bad)), nextHeader);
            in.remove(0, bad.length());
            // and continue
        }
    }
    return QPair<int, int>(-1, -1); // No more data, or not enough for a complete header
}

void CodaDevice::deviceReadyReadSerial()
{
    do {
        // Extract message (pos,len)
        const QPair<int, int> messagePos = findSerialHeader(d->m_readBuffer);
        if (messagePos.first < 0)
            break;
        // Do we have the complete message?
        const int messageEnd = messagePos.first + messagePos.second;
        if (messageEnd > d->m_readBuffer.size())
            break;
        processSerialMessage(d->m_readBuffer.mid(messagePos.first, messagePos.second));
        d->m_readBuffer.remove(0, messageEnd);
    } while (!d->m_readBuffer.isEmpty());
    checkSendQueue(); // Send off further messages
}

void CodaDevice::processSerialMessage(const QByteArray &message)
{
    if (debug > 1)
        qDebug("Serial message: %s",qPrintable(Coda::stringFromArray(message)));
    if (message.isEmpty())
        return;
    // Is thing a ping/pong response
    const int size = message.size();
    if (message.startsWith(serialPongC)) {
        const QString version = QString::fromLatin1(message.mid(sizeof(serialPongC) -  1));
        emitLogMessage(QString::fromLatin1("Serial connection from '%1'").arg(version));
        emit serialPong(version);
        // Answer with locator.
        if (!d->m_serialPingOnly)
            writeMessage(QByteArray(locatorAnswerC, sizeof(locatorAnswerC)));
        return;
    }
    // Check for long message (see top, '0xfe #number, data' or '0x0 #number, data')
    // TODO: This is currently untested.
    const unsigned char *dataU = reinterpret_cast<const unsigned char *>(message.constData());
    const bool isLongMessageStart        = size > SerialChunkHeaderSize
                                           && *dataU == serialChunkingStart;
    const bool isLongMessageContinuation = size > SerialChunkHeaderSize
                                           && *dataU == serialChunkingContinuation;
    if (isLongMessageStart || isLongMessageContinuation) {
        const unsigned chunkNumber = *++dataU;
        if (isLongMessageStart) { // Start new buffer
            d->m_serialBuffer.clear();
            d->m_serialBuffer.reserve( (chunkNumber + 1) * serialChunkLength);
        }
        d->m_serialBuffer.append(message.mid(SerialChunkHeaderSize, size - SerialChunkHeaderSize));
        // Last chunk? -  Process
        if (!chunkNumber) {
            processMessage(d->m_serialBuffer);
            d->m_serialBuffer.clear();
            d->m_serialBuffer.squeeze();
        }
    } else {
        processMessage(message); // Normal, unchunked message
    }
}

void CodaDevice::deviceReadyReadTcp()
{
    // Take complete message off front of readbuffer.
    do {
        const int messageEndPos = d->m_readBuffer.indexOf(d->m_tcpMessageTerminator);
        if (messageEndPos == -1)
            break;
        if (messageEndPos == 0) {
            // CODA 4.0.5 emits empty messages on errors.
            emitLogMessage(QString::fromLatin1("An empty CODA message has been received."));
        } else {
            processMessage(d->m_readBuffer.left(messageEndPos));
        }
        d->m_readBuffer.remove(0, messageEndPos + d->m_tcpMessageTerminator.size());
    } while (!d->m_readBuffer.isEmpty());
    checkSendQueue(); // Send off further messages
}

void CodaDevice::processMessage(const QByteArray &message)
{
    if (debug)
        qDebug("Read %d bytes:\n%s", message.size(), qPrintable(formatData(message)));
    if (const int errorCode = parseMessage(message)) {
        emitLogMessage(QString::fromLatin1("Parse error %1 : %2").
                       arg(errorCode).arg(debugMessage(message)));
        if (debug)
            qDebug("Parse error %d for %d bytes:\n%s", errorCode,
                   message.size(), qPrintable(formatData(message)));
    }
}

// Split \0-terminated message into tokens, skipping the initial type character
static inline QVector<QByteArray> splitMessage(const QByteArray &message)
{
    QVector<QByteArray> tokens;
    tokens.reserve(7);
    const int messageSize = message.size();
    for (int pos = 2; pos < messageSize; ) {
        const int nextPos = message.indexOf('\0', pos);
        if (nextPos == -1)
            break;
        tokens.push_back(message.mid(pos, nextPos - pos));
        pos = nextPos + 1;
    }
    return tokens;
}

int CodaDevice::parseMessage(const QByteArray &message)
{
    if (d->m_verbose)
        emitLogMessage(debugMessage(message, "CODA ->"));
    // Special JSON parse error message or protocol format error.
    // The port is usually closed after receiving it.
    // "\3\2{"Time":1276096098255,"Code":3,"Format": "Protocol format error"}"
    if (message.startsWith("\003\002")) {
        QByteArray text = message.mid(2);
        const QString errorMessage = QString::fromLatin1("Parse error received: %1").arg(QString::fromAscii(text));
        emit error(errorMessage);
        return 0;
    }
    if (message.size() < 4 || message.at(1) != '\0')
        return 1;
    // Split into tokens
    const char type = message.at(0);
    const QVector<QByteArray> tokens = splitMessage(message);
    switch (type) {
    case 'E':
        return parseCodaEvent(tokens);
    case 'R': // Command replies
    case 'N':
    case 'P':
        return parseCodaCommandReply(type, tokens);
    default:
        emitLogMessage(QString::fromLatin1("Unhandled message type: %1").arg(debugMessage(message)));
        return 756;
    }
    return 0;
}

int CodaDevice::parseCodaCommandReply(char type, const QVector<QByteArray> &tokens)
{
    typedef CodaDevicePrivate::TokenWrittenMessageMap::iterator TokenWrittenMessageMapIterator;
    // Find the corresponding entry in the written messages hash.
    const int tokenCount = tokens.size();
    if (tokenCount < 1)
        return 234;
    bool tokenOk;
    const int token = tokens.at(0).toInt(&tokenOk);
    if (!tokenOk)
        return 235;
    const TokenWrittenMessageMapIterator it = d->m_writtenMessages.find(token);
    if (it == d->m_writtenMessages.end()) {
        qWarning("CodaDevice: Internal error: token %d not found for '%s'",
                 token, qPrintable(joinByteArrays(tokens)));
        return 236;
    }

    CodaSendQueueEntry entry = it.value(); // FIXME: const?
    d->m_writtenMessages.erase(it);

    // No callback: remove entry from map, happy
    const unsigned specialHandling = entry.specialHandling;
    if (!entry.callback && specialHandling == 0u)
        return 0;

    // Parse values into JSON
    QVector<JsonValue> values;
    values.reserve(tokenCount);
    for (int i = 1; i < tokenCount; i++) {
        if (!tokens.at(i).isEmpty()) { // Strange: Empty tokens occur.
            const JsonValue value(tokens.at(i));
            if (value.isValid()) {
                values.push_back(value);
            } else {
                qWarning("JSON parse error for reply to command token %d: #%d '%s'",
                         token, i, tokens.at(i).constData());
                return -1;
            }
        }
    }
    // Construct result and invoke callback, remove entry from map.
    CodaCommandResult result(type, entry.service, entry.data, values, entry.cookie);
    if (entry.callback)
        entry.callback(result);

    return 0;
}

int CodaDevice::parseCodaEvent(const QVector<QByteArray> &tokens)
{
    // Event: Ignore the periodical heartbeat event, answer 'Hello',
    // emit signal for the rest
    if (tokens.size() < 3)
        return 433;
    const Services service = serviceFromName(tokens.at(0).constData());
    if (service == LocatorService && tokens.at(1) == "peerHeartBeat")
        return 0;
    QVector<JsonValue> values;
    for (int i = 2; i < tokens.size(); i++) {
        const JsonValue value(tokens.at(i));
        if (!value.isValid())
            return 434;
        values.push_back(value);
    }
    // Parse known events, emit signals
    QScopedPointer<CodaEvent> knownEvent(CodaEvent::parseEvent(service, tokens.at(1), values));
    if (!knownEvent.isNull()) {
        // Answer hello event (WLAN)
        if (knownEvent->type() == CodaEvent::LocatorHello)
            if (!d->m_serialFrame)
                writeMessage(QByteArray(locatorAnswerC, sizeof(locatorAnswerC)));
        emit codaEvent(*knownEvent);
    }
    emit genericCodaEvent(service, tokens.at(1), values);

    if (debug || d->m_verbose) {
        QString msg;
        QTextStream str(&msg);
        if (knownEvent.isNull()) {
            str << "Event: " << tokens.at(0) << ' ' << tokens.at(1) << '\n';
            foreach(const JsonValue &val, values)
                str << "  " << val.toString() << '\n';
        } else {
            str << knownEvent->toString();
        }
        emitLogMessage(msg);
    }

    return 0;
}

unsigned CodaDevice::verbose() const
{
    return d->m_verbose;
}

bool CodaDevice::serialFrame() const
{
    return d->m_serialFrame;
}

void CodaDevice::setSerialFrame(bool s)
{
    d->m_serialFrame = s;
}

void CodaDevice::setVerbose(unsigned v)
{
    d->m_verbose = v;
}

void CodaDevice::emitLogMessage(const QString &m)
{
    if (debug)
        qWarning("%s", qPrintable(m));
    emit logMessage(m);
}

bool CodaDevice::checkOpen()
{
    if (d->m_device.isNull()) {
        emitLogMessage(QLatin1String("Internal error: No device set on CodaDevice."));
        return false;
    }
    if (!d->m_device->isOpen()) {
        emitLogMessage(QLatin1String("Internal error: Device not open in CodaDevice."));
        return false;
    }
    return true;
}

void CodaDevice::sendSerialPing(bool pingOnly)
{
    if (!checkOpen())
        return;

    d->m_serialPingOnly = pingOnly;
    setSerialFrame(true);
    writeMessage(QByteArray(serialPingC, qstrlen(serialPingC)), false);
    if (d->m_verbose)
        emitLogMessage(QLatin1String("Ping..."));
}

void CodaDevice::sendCodaMessage(MessageType mt, Services service, const char *command,
                                     const char *commandParameters, // may contain '\0'
                                     int commandParametersLength,
                                     const CodaCallback &callBack,
                                     const QVariant &cookie)

{
    if (!checkOpen())
        return;
    // Format the message
    const int  token = d->m_token++;
    QByteArray data;
    data.reserve(30 + commandParametersLength);
    data.append('C');
    data.append('\0');
    data.append(QByteArray::number(token));
    data.append('\0');
    data.append(serviceName(service));
    data.append('\0');
    data.append(command);
    data.append('\0');
    if (commandParametersLength)
        data.append(commandParameters, commandParametersLength);
    const CodaSendQueueEntry entry(mt, token, service, data, callBack, cookie);
    d->m_sendQueue.enqueue(entry);
    checkSendQueue();
}

void CodaDevice::sendCodaMessage(MessageType mt, Services service, const char *command,
                                     const QByteArray &commandParameters,
                                     const CodaCallback &callBack,
                                     const QVariant &cookie)
{
    sendCodaMessage(mt, service, command, commandParameters.constData(), commandParameters.size(),
                      callBack, cookie);
}

// Enclose in message frame and write.
void CodaDevice::writeMessage(QByteArray data, bool ensureTerminating0)
{
    if (!checkOpen())
        return;

    if (d->m_serialFrame && data.size() > maxSerialMessageLength) {
        qCritical("Attempt to send large message (%d bytes) exceeding the "
                  "limit of %d bytes over serial channel. Skipping.",
                  data.size(), maxSerialMessageLength);
        return;
    }

    if (d->m_verbose)
        emitLogMessage(debugMessage(data, "CODA <-"));

    // Ensure \0-termination which easily gets lost in QByteArray CT.
    if (ensureTerminating0 && !data.endsWith('\0'))
        data.append('\0');
    if (d->m_serialFrame) {
        data = encodeUsbSerialMessage(data);
    } else {
        data += d->m_tcpMessageTerminator;
    }

    if (debug > 1)
        qDebug("Writing:\n%s", qPrintable(formatData(data)));

    int result = d->m_device->write(data);
    if (result < data.length())
        qWarning("Failed to write all data! result=%d", result);
    if (QAbstractSocket *as = qobject_cast<QAbstractSocket *>(d->m_device.data()))
        as->flush();
}

void CodaDevice::writeCustomData(char protocolId, const QByteArray &data)
{
    if (!checkOpen())
        return;

    if (!d->m_serialFrame) {
        qWarning("Ignoring request to send data to non-serial CodaDevice");
        return;
    }
    if (data.length() > 0xFFFF) {
        qWarning("Ignoring request to send too large packet, of size %d", data.length());
        return;
    }
    QByteArray framedData;
    encodeSerialFrame(data, &framedData, protocolId);
    device()->write(framedData);
}

void CodaDevice::checkSendQueue()
{
    // Fire off messages or invoke noops until a message with reply is found
    // and an entry to writtenMessages is made.
    while (d->m_writtenMessages.empty()) {
        if (d->m_sendQueue.isEmpty())
            break;
        CodaSendQueueEntry entry = d->m_sendQueue.dequeue();
        switch (entry.messageType) {
        case MessageWithReply:
            d->m_writtenMessages.insert(entry.token, entry);
            writeMessage(entry.data);
            break;
        case MessageWithoutReply:
            writeMessage(entry.data);
            break;
        case NoopMessage: // Invoke the noop-callback for synchronization
            if (entry.callback) {
                CodaCommandResult noopResult(CodaCommandResult::SuccessReply);
                noopResult.cookie = entry.cookie;
                entry.callback(noopResult);
            }
            break;
        }
    }
}

// Fix slashes
static inline QString fixFileName(QString in)
{
    in.replace(QLatin1Char('/'), QLatin1Char('\\'));
    return in;
}

// Start a process (consisting of a non-reply setSettings and start).
void CodaDevice::sendProcessStartCommand(const CodaCallback &callBack,
                                                 const QString &binaryIn,
                                                 unsigned uid,
                                                 QStringList arguments,
                                                 QString workingDirectory,
                                                 bool debugControl,
                                                 const QStringList &additionalLibraries,
                                                 const QVariant &cookie)
{
    // Obtain the bin directory, expand by c:/sys/bin if missing
    const QChar backSlash('\\');
    int slashPos = binaryIn.lastIndexOf(QLatin1Char('/'));
    if (slashPos == -1)
        slashPos = binaryIn.lastIndexOf(backSlash);
    const QString sysBin = QLatin1String("c:/sys/bin");
    const QString binaryFileName  = slashPos == -1 ? binaryIn : binaryIn.mid(slashPos + 1);

    if (workingDirectory.isEmpty())
        workingDirectory = sysBin;

    // Format settings with empty dummy parameter
    QByteArray setData;
    JsonInputStream setStr(setData);
    setStr << "" << '\0'
           << '[' << "exeToLaunch" << ',' << "addExecutables" << ',' << "addLibraries" << ',' << "logUserTraces" << ',' << "attachAllWithLibraries" << ']'
           << '\0' << '['
                << binaryFileName << ','
                << '{' << binaryFileName << ':' << QString::number(uid, 16) << '}' << ','
                << additionalLibraries << ',' << true << ',' << false
           << ']';
    sendCodaMessage(
#if 1
                MessageWithReply,    // CODA 4.0.5 onwards
#else
                MessageWithoutReply, // CODA 4.0.2
#endif
                SettingsService, "set", setData);

    QByteArray startData;
    JsonInputStream startStr(startData);
    startStr << "" //We don't really know the drive of the working dir
            << '\0' << binaryFileName << '\0' << arguments << '\0'
            << QStringList() << '\0' // Env is an array ["PATH=value"] (non-standard)
            << debugControl;
    sendCodaMessage(MessageWithReply, ProcessesService, "start", startData, callBack, cookie);
}

void CodaDevice::sendRunProcessCommand(const CodaCallback &callBack,
                                       const QString &processName,
                                       QStringList arguments,
                                       const QVariant &cookie)
{
    QByteArray startData;
    JsonInputStream startStr(startData);
    startStr << "" //We don't really know the drive of the working dir
            << '\0' << processName << '\0' << arguments << '\0'
            << QStringList() << '\0' // Env is an array ["PATH=value"] (non-standard)
            << false; // Don't attach debugger
    sendCodaMessage(MessageWithReply, ProcessesService, "start", startData, callBack, cookie);
}

void CodaDevice::sendSettingsEnableLogCommand()
{

    QByteArray setData;
    JsonInputStream setStr(setData);
    setStr << "" << '\0'
            << '[' << "logUserTraces" << ']'
            << '\0' << '['
            << true
            << ']';
    sendCodaMessage(
#if 1
                MessageWithReply,    // CODA 4.0.5 onwards
#else
                MessageWithoutReply, // CODA 4.0.2
#endif
                SettingsService, "set", setData);
}

void CodaDevice::sendProcessTerminateCommand(const CodaCallback &callBack,
                                               const QByteArray &id,
                                               const QVariant &cookie)
{
    QByteArray data;
    JsonInputStream str(data);
    str << id;
    sendCodaMessage(MessageWithReply, ProcessesService, "terminate", data, callBack, cookie);
}

void CodaDevice::sendRunControlTerminateCommand(const CodaCallback &callBack,
                                                  const QByteArray &id,
                                                  const QVariant &cookie)
{
    QByteArray data;
    JsonInputStream str(data);
    str << id;
    sendCodaMessage(MessageWithReply, RunControlService, "terminate", data, callBack, cookie);
}

// Non-standard: Remove executable from settings
void CodaDevice::sendSettingsRemoveExecutableCommand(const QString &binaryIn,
                                                       unsigned uid,
                                                       const QStringList &additionalLibraries,
                                                       const QVariant &cookie)
{
    QByteArray setData;
    JsonInputStream setStr(setData);
    setStr << "" << '\0'
            << '[' << "removedExecutables" << ',' << "removedLibraries" << ']'
            << '\0' << '['
                << '{' << QFileInfo(binaryIn).fileName() << ':' << QString::number(uid, 16) << '}' << ','
                << additionalLibraries
            << ']';
    sendCodaMessage(MessageWithoutReply, SettingsService, "set", setData, CodaCallback(), cookie);
}

void CodaDevice::sendRunControlResumeCommand(const CodaCallback &callBack,
                                               const QByteArray &id,
                                               RunControlResumeMode mode,
                                               unsigned count,
                                               quint64 rangeStart,
                                               quint64 rangeEnd,
                                               const QVariant &cookie)
{
    QByteArray resumeData;
    JsonInputStream str(resumeData);
    str << id << '\0' << int(mode) << '\0' << count;
    switch (mode) {
    case RM_STEP_OVER_RANGE:
    case RM_STEP_INTO_RANGE:
    case RM_REVERSE_STEP_OVER_RANGE:
    case RM_REVERSE_STEP_INTO_RANGE:
        str << '\0' << '{' << "RANGE_START" << ':' << rangeStart
                << ',' << "RANGE_END" << ':' << rangeEnd << '}';
        break;
    default:
        break;
    }
    sendCodaMessage(MessageWithReply, RunControlService, "resume", resumeData, callBack, cookie);
}

void CodaDevice::sendRunControlSuspendCommand(const CodaCallback &callBack,
                                                const QByteArray &id,
                                                const QVariant &cookie)
{
    QByteArray data;
    JsonInputStream str(data);
    str << id;
    sendCodaMessage(MessageWithReply, RunControlService, "suspend", data, callBack, cookie);
}

void CodaDevice::sendRunControlResumeCommand(const CodaCallback &callBack,
                                               const QByteArray &id,
                                               const QVariant &cookie)
{
    sendRunControlResumeCommand(callBack, id, RM_RESUME, 1, 0, 0, cookie);
}

void CodaDevice::sendBreakpointsAddCommand(const CodaCallback &callBack,
                                             const Breakpoint &bp,
                                             const QVariant &cookie)
{
    QByteArray data;
    JsonInputStream str(data);
    str << bp;
    sendCodaMessage(MessageWithReply, BreakpointsService, "add", data, callBack, cookie);
}

void CodaDevice::sendBreakpointsRemoveCommand(const CodaCallback &callBack,
                                                const QByteArray &id,
                                                const QVariant &cookie)
{
    sendBreakpointsRemoveCommand(callBack, QVector<QByteArray>(1, id), cookie);
}

void CodaDevice::sendBreakpointsRemoveCommand(const CodaCallback &callBack,
                                                const QVector<QByteArray> &ids,
                                                const QVariant &cookie)
{
    QByteArray data;
    JsonInputStream str(data);
    str << ids;
    sendCodaMessage(MessageWithReply, BreakpointsService, "remove", data, callBack, cookie);
}

void CodaDevice::sendBreakpointsEnableCommand(const CodaCallback &callBack,
                                                const QByteArray &id,
                                                bool enable,
                                                const QVariant &cookie)
{
    sendBreakpointsEnableCommand(callBack, QVector<QByteArray>(1, id), enable, cookie);
}

void CodaDevice::sendBreakpointsEnableCommand(const CodaCallback &callBack,
                                                const QVector<QByteArray> &ids,
                                                bool enable,
                                                const QVariant &cookie)
{
    QByteArray data;
    JsonInputStream str(data);
    str << ids;
    sendCodaMessage(MessageWithReply, BreakpointsService,
                      enable ? "enable" : "disable",
                      data, callBack, cookie);
}

void CodaDevice::sendMemorySetCommand(const CodaCallback &callBack,
                                        const QByteArray &contextId,
                                        quint64 start, const QByteArray& data,
                                        const QVariant &cookie)
{
    QByteArray getData;
    JsonInputStream str(getData);
    // start/word size/mode. Mode should ideally be 1 (continue on error?)
    str << contextId << '\0' << start << '\0' << 1 << '\0' << data.size() << '\0' << 1
        << '\0' << data.toBase64();
    sendCodaMessage(MessageWithReply, MemoryService, "set", getData, callBack, cookie);
}

void CodaDevice::sendMemoryGetCommand(const CodaCallback &callBack,
                                        const QByteArray &contextId,
                                        quint64 start, quint64 size,
                                        const QVariant &cookie)
{
    QByteArray data;
    JsonInputStream str(data);
    // start/word size/mode. Mode should ideally be 1 (continue on error?)
    str << contextId << '\0' << start << '\0' << 1 << '\0' << size << '\0' << 1;
    sendCodaMessage(MessageWithReply, MemoryService, "get", data, callBack, cookie);
}

QByteArray CodaDevice::parseMemoryGet(const CodaCommandResult &r)
{
    if (r.type != CodaCommandResult::SuccessReply || r.values.size() < 1)
        return QByteArray();
    const JsonValue &memoryV = r.values.front();

    if (memoryV.type() != JsonValue::String || memoryV.data().size() < 2
        || !memoryV.data().endsWith('='))
        return QByteArray();
    // Catch errors reported as hash:
    // R.4."TlVMTA==".{"Time":1276786871255,"Code":1,"AltCode":-38,"AltOrg":"POSIX","Format":"BadDescriptor"}
    // Not sure what to make of it.
    if (r.values.size() >= 2 && r.values.at(1).type() == JsonValue::Object)
        qWarning("CodaDevice::parseMemoryGet(): Error retrieving memory: %s", r.values.at(1).toString(false).constData());
    // decode
    const QByteArray memory = QByteArray::fromBase64(memoryV.data());
    if (memory.isEmpty())
        qWarning("Base64 decoding of %s failed.", memoryV.data().constData());
    if (debug)
        qDebug("CodaDevice::parseMemoryGet: received %d bytes", memory.size());
    return memory;
}

// Parse register children (array of names)
QVector<QByteArray> CodaDevice::parseRegisterGetChildren(const CodaCommandResult &r)
{
    QVector<QByteArray> rc;
    if (!r || r.values.size() < 1 || r.values.front().type() != JsonValue::Array)
        return rc;
    const JsonValue &front = r.values.front();
    rc.reserve(front.childCount());
    foreach(const JsonValue &v, front.children())
        rc.push_back(v.data());
    return rc;
}

CodaStatResponse CodaDevice::parseStat(const CodaCommandResult &r)
{
    CodaStatResponse rc;
    if (!r || r.values.size() < 1 || r.values.front().type() != JsonValue::Object)
        return rc;
    foreach(const JsonValue &v, r.values.front().children()) {
        if (v.name() == "Size") {
            rc.size = v.data().toULongLong();
        } else if (v.name() == "ATime") {
            if (const quint64 atime = v.data().toULongLong())
                rc.accessTime = CodaCommandResult::codaTimeToQDateTime(atime);
        } else if (v.name() == "MTime") {
            if (const quint64 mtime = v.data().toULongLong())
                rc.modTime = CodaCommandResult::codaTimeToQDateTime(mtime);
        }
    }
    return rc;
}

void CodaDevice::sendRegistersGetChildrenCommand(const CodaCallback &callBack,
                                     const QByteArray &contextId,
                                     const QVariant &cookie)
{
    QByteArray data;
    JsonInputStream str(data);
    str << contextId;
    sendCodaMessage(MessageWithReply, RegistersService, "getChildren", data, callBack, cookie);
}

// Format id of register get request (needs contextId containing process and thread)
static inline QByteArray registerId(const QByteArray &contextId, QByteArray id)
{
    QByteArray completeId = contextId;
    if (!completeId.isEmpty())
        completeId.append('.');
    completeId.append(id);
    return completeId;
}

// Format parameters of register get request
static inline QByteArray registerGetData(const QByteArray &contextId, QByteArray id)
{
    QByteArray data;
    JsonInputStream str(data);
    str << registerId(contextId, id);
    return data;
}

void CodaDevice::sendRegistersGetCommand(const CodaCallback &callBack,
                                            const QByteArray &contextId,
                                            QByteArray id,
                                            const QVariant &cookie)
{
    sendCodaMessage(MessageWithReply, RegistersService, "get",
                      registerGetData(contextId, id), callBack, cookie);
}

void CodaDevice::sendRegistersGetMCommand(const CodaCallback &callBack,
                                            const QByteArray &contextId,
                                            const QVector<QByteArray> &ids,
                                            const QVariant &cookie)
{
    // Format the register ids as a JSON list
    QByteArray data;
    JsonInputStream str(data);
    str << '[';
    const int count = ids.size();
    for (int r = 0; r < count; r++) {
        if (r)
            str << ',';
        // TODO: When 8-byte floating-point registers are supported, query for register length based on register id
        str << '[' << registerId(contextId, ids.at(r)) << ',' << '0' << ',' << '4' << ']';
    }
    str << ']';
    sendCodaMessage(MessageWithReply, RegistersService, "getm", data, callBack, cookie);
}

void CodaDevice::sendRegistersGetMRangeCommand(const CodaCallback &callBack,
                                                 const QByteArray &contextId,
                                                 unsigned start, unsigned count)
{
    const unsigned end = start + count;
    if (end > (unsigned)d->m_registerNames.size()) {
        qWarning("CodaDevice: No register name set for index %u (size: %d).", end, d->m_registerNames.size());
        return;
    }

    QVector<QByteArray> ids;
    ids.reserve(count);
    for (unsigned i = start; i < end; ++i)
        ids.push_back(d->m_registerNames.at(i));
    sendRegistersGetMCommand(callBack, contextId, ids, QVariant(start));
}

// Set register
void CodaDevice::sendRegistersSetCommand(const CodaCallback &callBack,
                                           const QByteArray &contextId,
                                           QByteArray id,
                                           const QByteArray &value,
                                           const QVariant &cookie)
{
    QByteArray data;
    JsonInputStream str(data);
    if (!contextId.isEmpty()) {
        id.prepend('.');
        id.prepend(contextId);
    }
    str << id << '\0' << value.toBase64();
    sendCodaMessage(MessageWithReply, RegistersService, "set", data, callBack, cookie);
}

// Set register
void CodaDevice::sendRegistersSetCommand(const CodaCallback &callBack,
                                           const QByteArray &contextId,
                                           unsigned registerNumber,
                                           const QByteArray &value,
                                           const QVariant &cookie)
{
    if (registerNumber >= (unsigned)d->m_registerNames.size()) {
        qWarning("CodaDevice: No register name set for index %u (size: %d).", registerNumber, d->m_registerNames.size());
        return;
    }
    sendRegistersSetCommand(callBack, contextId,
                            d->m_registerNames[registerNumber],
                            value, cookie);
}

static const char outputListenerIDC[] = "ProgramOutputConsoleLogger";

void CodaDevice::sendLoggingAddListenerCommand(const CodaCallback &callBack,
                                                 const QVariant &cookie)
{
    QByteArray data;
    JsonInputStream str(data);
    str << outputListenerIDC;
    sendCodaMessage(MessageWithReply, LoggingService, "addListener", data, callBack, cookie);
}

void CodaDevice::sendSymbianUninstallCommand(const Coda::CodaCallback &callBack,
                                             const quint32 package,
                                             const QVariant &cookie)
{
    QByteArray data;
    JsonInputStream str(data);
    QString string = QString::number(package, 16);
    str << string;
    sendCodaMessage(MessageWithReply, SymbianInstallService, "uninstall", data, callBack, cookie);
}

void CodaDevice::sendSymbianOsDataGetThreadsCommand(const CodaCallback &callBack,
                                                 const QVariant &cookie)
{
    QByteArray data;
    sendCodaMessage(MessageWithReply, SymbianOSData, "getThreads", data, callBack, cookie);
}

void CodaDevice::sendSymbianOsDataFindProcessesCommand(const CodaCallback &callBack,
                                            const QByteArray &processName,
                                            const QByteArray &uid,
                                            const QVariant &cookie)
{
    QByteArray data;
    JsonInputStream str(data);
    str << processName << '\0' << uid;
    sendCodaMessage(MessageWithReply, SymbianOSData, "findRunningProcesses", data, callBack, cookie);
}

void CodaDevice::sendSymbianOsDataGetQtVersionCommand(const CodaCallback &callBack,
                                                      const QVariant &cookie)
{
    sendCodaMessage(MessageWithReply, SymbianOSData, "getQtVersion", QByteArray(), callBack, cookie);
}

void CodaDevice::sendSymbianOsDataGetRomInfoCommand(const CodaCallback &callBack,
                                                      const QVariant &cookie)
{
    sendCodaMessage(MessageWithReply, SymbianOSData, "getRomInfo", QByteArray(), callBack, cookie);
}

void CodaDevice::sendSymbianOsDataGetHalInfoCommand(const CodaCallback &callBack,
                                                    const QStringList &keys,
                                                    const QVariant &cookie)
{
    QByteArray data;
    JsonInputStream str(data);
    str << '[';
    for (int i = 0; i < keys.count(); ++i) {
        if (i)
            str << ',';
        str << keys[i];
    }
    str << ']';
    sendCodaMessage(MessageWithReply, SymbianOSData, "getHalInfo", data, callBack, cookie);
}

void Coda::CodaDevice::sendFileSystemOpenCommand(const Coda::CodaCallback &callBack,
                                                     const QByteArray &name,
                                                     unsigned flags,
                                                     const QVariant &cookie)
{
    QByteArray data;
    JsonInputStream str(data);
    str << name << '\0' << flags << '\0' << '{' << '}';
    sendCodaMessage(MessageWithReply, FileSystemService, "open", data, callBack, cookie);
}

void Coda::CodaDevice::sendFileSystemFstatCommand(const CodaCallback &callBack,
                                                      const QByteArray &handle,
                                                      const QVariant &cookie)
{
    QByteArray data;
    JsonInputStream str(data);
    str << handle;
    sendCodaMessage(MessageWithReply, FileSystemService, "fstat", data, callBack, cookie);
}

void Coda::CodaDevice::sendFileSystemWriteCommand(const Coda::CodaCallback &callBack,
                                                      const QByteArray &handle,
                                                      const QByteArray &dataIn,
                                                      unsigned offset,
                                                      const QVariant &cookie)
{
    QByteArray data;
    JsonInputStream str(data);
    str << handle << '\0' << offset << '\0' << dataIn.toBase64();
    sendCodaMessage(MessageWithReply, FileSystemService, "write", data, callBack, cookie);
}

void Coda::CodaDevice::sendFileSystemCloseCommand(const Coda::CodaCallback &callBack,
                                                      const QByteArray &handle,
                                                      const QVariant &cookie)
{
    QByteArray data;
    JsonInputStream str(data);
    str << handle;
    sendCodaMessage(MessageWithReply, FileSystemService, "close", data, callBack, cookie);
}

void Coda::CodaDevice::sendSymbianInstallSilentInstallCommand(const Coda::CodaCallback &callBack,
                                                                  const QByteArray &file,
                                                                  const QByteArray &targetDrive,
                                                                  const QVariant &cookie)
{
    QByteArray data;
    JsonInputStream str(data);
    str << file << '\0' << targetDrive;
    sendCodaMessage(MessageWithReply, SymbianInstallService, "install", data, callBack, cookie);
}

void Coda::CodaDevice::sendSymbianInstallUIInstallCommand(const Coda::CodaCallback &callBack,
                                                              const QByteArray &file,
                                                              const QVariant &cookie)
{
    QByteArray data;
    JsonInputStream str(data);
    str << file;
    sendCodaMessage(MessageWithReply, SymbianInstallService, "installWithUI", data, callBack, cookie);
}

void Coda::CodaDevice::sendSymbianInstallGetPackageInfoCommand(const Coda::CodaCallback &callBack,
                                                               const QList<quint32> &packages,
                                                               const QVariant &cookie)
{
    QByteArray data;
    JsonInputStream str(data);
    str << '[';
    for (int i = 0; i < packages.count(); ++i) {
        if (i)
            str << ',';
        QString pkgString;
        pkgString.setNum(packages[i], 16);
        str << pkgString;
    }
    str << ']';
    sendCodaMessage(MessageWithReply, SymbianInstallService, "getPackageInfo", data, callBack, cookie);
}

void  Coda::CodaDevice::sendDebugSessionControlSessionStartCommand(const Coda::CodaCallback &callBack,
                                                                   const QVariant &cookie)
{
    sendCodaMessage(MessageWithReply, DebugSessionControl, "sessionStart", QByteArray(), callBack, cookie);
}

void  Coda::CodaDevice::sendDebugSessionControlSessionEndCommand(const Coda::CodaCallback &callBack,
                                                                 const QVariant &cookie)
{
    sendCodaMessage(MessageWithReply, DebugSessionControl, "sessionEnd ", QByteArray(), callBack, cookie);
}

} // namespace Coda
