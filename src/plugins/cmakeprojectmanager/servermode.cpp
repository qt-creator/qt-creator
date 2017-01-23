/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "servermode.h"

#include <coreplugin/icore.h>
#include <coreplugin/reaper.h>

#include <utils/algorithm.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>
#include <utils/temporarydirectory.h>

#include <QByteArray>
#include <QCryptographicHash>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocalSocket>
#include <QUuid>

using namespace Utils;

namespace CMakeProjectManager {
namespace Internal {

const char COOKIE_KEY[] = "cookie";
const char IN_REPLY_TO_KEY[] = "inReplyTo";
const char NAME_KEY[] = "name";
const char TYPE_KEY[] = "type";

const char ERROR_TYPE[] = "error";
const char HANDSHAKE_TYPE[] = "handshake";

const char START_MAGIC[] = "\n[== \"CMake Server\" ==[\n";
const char END_MAGIC[] = "\n]== \"CMake Server\" ==]\n";

Q_LOGGING_CATEGORY(cmakeServerMode, "qtc.cmake.serverMode");

// ----------------------------------------------------------------------
// Helpers:
// ----------------------------------------------------------------------

bool isValid(const QVariant &v)
{
    if (v.isNull())
        return false;
    if (v.type() == QVariant::String) // Workaround signals sending an empty string cookie
        return !v.toString().isEmpty();
    return true;
}

// --------------------------------------------------------------------
// ServerMode:
// --------------------------------------------------------------------


ServerMode::ServerMode(const Environment &env,
                       const FileName &sourceDirectory, const FileName &buildDirectory,
                       const FileName &cmakeExecutable,
                       const QString &generator, const QString &extraGenerator,
                       const QString &platform, const QString &toolset,
                       bool experimental, int major, int minor,
                       QObject *parent) :
    QObject(parent),
#if defined(Q_OS_UNIX)
    // Some unixes (e.g. Darwin) limit the length of a local socket to about 100 char (or less).
    // Since some unixes (e.g. Darwin) also point TMPDIR to /really/long/paths we need to create
    // our own socket in a place closer to '/'.
    m_socketDir("/tmp/cmake-"),
#endif
    m_sourceDirectory(sourceDirectory), m_buildDirectory(buildDirectory),
    m_cmakeExecutable(cmakeExecutable),
    m_generator(generator), m_extraGenerator(extraGenerator),
    m_platform(platform), m_toolset(toolset),
    m_useExperimental(experimental), m_majorProtocol(major), m_minorProtocol(minor)
{
    QTC_ASSERT(!m_sourceDirectory.isEmpty() && m_sourceDirectory.exists(), return);
    QTC_ASSERT(!m_buildDirectory.isEmpty() && m_buildDirectory.exists(), return);

    m_connectionTimer.setInterval(100);
    connect(&m_connectionTimer, &QTimer::timeout, this, &ServerMode::connectToServer);

    m_cmakeProcess.reset(new QtcProcess);

    m_cmakeProcess->setEnvironment(env);
    m_cmakeProcess->setWorkingDirectory(buildDirectory.toString());

#if defined(Q_OS_UNIX)
    m_socketName = m_socketDir.path() + "/socket";
#else
    m_socketName = QString::fromLatin1("\\\\.\\pipe\\") + QUuid::createUuid().toString();
#endif

    const QStringList args = QStringList({ "-E", "server", "--pipe=" + m_socketName });

    connect(m_cmakeProcess.get(), &QtcProcess::started, this, [this]() { m_connectionTimer.start(); });
    connect(m_cmakeProcess.get(),
            static_cast<void(QtcProcess::*)(int, QProcess::ExitStatus)>(&QtcProcess::finished),
            this, &ServerMode::handleCMakeFinished);

    QString argumentString;
    QtcProcess::addArgs(&argumentString, args);
    if (m_useExperimental)
        QtcProcess::addArg(&argumentString, "--experimental");

    qCInfo(cmakeServerMode)
            << "Preparing cmake:" << cmakeExecutable.toString() << argumentString
            << "in" << m_buildDirectory.toString();
    m_cmakeProcess->setCommand(cmakeExecutable.toString(), argumentString);

    // Delay start:
    QTimer::singleShot(0, this, [argumentString, this] {
        emit message(tr("Running \"%1 %2\" in %3.")
                     .arg(m_cmakeExecutable.toUserOutput())
                     .arg(argumentString)
                     .arg(m_buildDirectory.toUserOutput()));

        m_cmakeProcess->start();
    });
}

ServerMode::~ServerMode()
{
    if (m_cmakeProcess)
        m_cmakeProcess->disconnect();
    if (m_cmakeSocket) {
        m_cmakeSocket->disconnect();
        m_cmakeSocket->abort();
        delete(m_cmakeSocket);
    }
    m_cmakeSocket = nullptr;
    Core::Reaper::reap(m_cmakeProcess.release());

    qCDebug(cmakeServerMode) << "Server-Mode closed.";
}

void ServerMode::sendRequest(const QString &type, const QVariantMap &extra, const QVariant &cookie)
{
    QTC_ASSERT(m_cmakeSocket, return);
    ++m_requestCounter;

    qCInfo(cmakeServerMode) << "Sending Request" << type << "(" << cookie << ")";

    QVariantMap data = extra;
    data.insert(TYPE_KEY, type);
    const QVariant realCookie = cookie.isNull() ? QVariant(m_requestCounter) : cookie;
    data.insert(COOKIE_KEY, realCookie);
    m_expectedReplies.push_back({ type, realCookie });

    QJsonObject object = QJsonObject::fromVariantMap(data);
    QJsonDocument document;
    document.setObject(object);

    const QByteArray rawData = START_MAGIC + document.toJson(QJsonDocument::Compact) + END_MAGIC;
    qCDebug(cmakeServerMode) << ">>>" << rawData;
    m_cmakeSocket->write(rawData);
    m_cmakeSocket->flush();
}

bool ServerMode::isConnected()
{
    return m_cmakeSocket && m_isConnected;
}

void ServerMode::connectToServer()
{
    QTC_ASSERT(m_cmakeProcess, return);
    if (m_cmakeSocket)
        return; // We connected in the meantime...

    static int counter = 0;
    ++counter;

    if (counter > 50) {
        counter = 0;
        m_cmakeProcess->disconnect();
        qCInfo(cmakeServerMode) << "Timeout waiting for pipe" << m_socketName;
        reportError(tr("Running \"%1\" failed: Timeout waiting for pipe \"%2\".")
                    .arg(m_cmakeExecutable.toUserOutput())
                    .arg(m_socketName));

        Core::Reaper::reap(m_cmakeProcess.release());
        emit disconnected();
        return;
    }

    QTC_ASSERT(!m_cmakeSocket, return);

    auto socket = new QLocalSocket(m_cmakeProcess.get());
    connect(socket, &QLocalSocket::readyRead, this, &ServerMode::handleRawCMakeServerData);
    connect(socket, static_cast<void(QLocalSocket::*)(QLocalSocket::LocalSocketError)>(&QLocalSocket::error),
            this, [this, socket]() {
        reportError(socket->errorString());
        m_cmakeSocket = nullptr;
        socket->disconnect();
        socket->deleteLater();
    });
    connect(socket, &QLocalSocket::connected, this, [this, socket]() { m_cmakeSocket = socket; });
    connect(socket, &QLocalSocket::disconnected, this, [this, socket]() {
        if (m_cmakeSocket)
            emit disconnected();
        m_cmakeSocket = nullptr;
        socket->disconnect();
        socket->deleteLater();
    });

    socket->connectToServer(m_socketName);
    m_connectionTimer.start();
}

void ServerMode::handleCMakeFinished(int code, QProcess::ExitStatus status)
{
    qCInfo(cmakeServerMode) << "CMake has finished" << code << status;
    QString msg;
    if (status != QProcess::NormalExit)
        msg = tr("CMake process \"%1\" crashed.").arg(m_cmakeExecutable.toUserOutput());
    else if (code != 0)
        msg = tr("CMake process \"%1\" quit with exit code %2.").arg(m_cmakeExecutable.toUserOutput()).arg(code);

    if (!msg.isEmpty()) {
        reportError(msg);
    } else {
        emit message(tr("CMake process \"%1\" quit normally.").arg(m_cmakeExecutable.toUserOutput()));
    }

    if (m_cmakeSocket) {
        m_cmakeSocket->disconnect();
        delete m_cmakeSocket;
        m_cmakeSocket = nullptr;
    }

    if (!HostOsInfo::isWindowsHost())
        QFile::remove(m_socketName);

    emit disconnected();
}

void ServerMode::handleRawCMakeServerData()
{
    const static QByteArray startNeedle(START_MAGIC);
    const static QByteArray endNeedle(END_MAGIC);

    if (!m_cmakeSocket) // might happen during shutdown
        return;

    m_buffer.append(m_cmakeSocket->readAll());

    while (true) {
        const int startPos = m_buffer.indexOf(startNeedle);
        if (startPos >= 0) {
            const int afterStartNeedle = startPos + startNeedle.count();
            const int endPos = m_buffer.indexOf(endNeedle, afterStartNeedle);
            if (endPos > afterStartNeedle) {
                // Process JSON, remove junk and JSON-part, continue to loop with shorter buffer
                parseBuffer(m_buffer.mid(afterStartNeedle, endPos - afterStartNeedle));
                m_buffer.remove(0, endPos + endNeedle.count());
            } else {
                // Remove junk up to the start needle and break out of the loop
                if (startPos > 0)
                    m_buffer.remove(0, startPos);
                break;
            }
        } else {
            // Keep at last startNeedle.count() characters (as that might be a
            // partial startNeedle), break out of the loop
            if (m_buffer.count() > startNeedle.count())
                m_buffer.remove(0, m_buffer.count() - startNeedle.count());
            break;
        }
    }
}

void ServerMode::parseBuffer(const QByteArray &buffer)
{
    qCDebug(cmakeServerMode) << "<<<" << buffer;
    QJsonDocument document = QJsonDocument::fromJson(buffer);
    if (document.isNull()) {
        reportError(tr("Failed to parse JSON from CMake server."));
        return;
    }
    QJsonObject rootObject = document.object();
    if (rootObject.isEmpty()) {
        reportError(tr("JSON data from CMake server was not a JSON object."));
        return;
    }

    parseJson(rootObject.toVariantMap());
}

void ServerMode::parseJson(const QVariantMap &data)
{
    QString type = data.value(TYPE_KEY).toString();
    if (type == "hello") {
        qCInfo(cmakeServerMode) << "Got \"hello\" message.";
        if (m_gotHello) {
            reportError(tr("Unexpected hello received from CMake server."));
            return;
        } else {
            handleHello(data);
            m_gotHello = true;
            return;
        }
    }
    if (!m_gotHello && type != ERROR_TYPE) {
        reportError(tr("Unexpected type \"%1\" received while waiting for \"hello\".").arg(type));
        return;
    }

    if (type == "reply") {
        if (m_expectedReplies.empty()) {
            reportError(tr("Received a reply even though no request is open."));
            return;
        }
        const QString replyTo = data.value(IN_REPLY_TO_KEY).toString();
        const QVariant cookie = data.value(COOKIE_KEY);
        qCInfo(cmakeServerMode) << "Got \"reply\" message." << replyTo << "(" << cookie << ")";

        const auto expected = m_expectedReplies.begin();
        if (expected->type != replyTo) {
            reportError(tr("Received a reply to a request of type \"%1\", when a request of type \"%2\" was sent.")
                        .arg(replyTo).arg(expected->type));
            return;
        }
        if (expected->cookie != cookie) {
            reportError(tr("Received a reply with cookie \"%1\", when \"%2\" was expected.")
                        .arg(cookie.toString()).arg(expected->cookie.toString()));
            return;
        }

        m_expectedReplies.erase(expected);
        if (replyTo != HANDSHAKE_TYPE)
            emit cmakeReply(data, replyTo, cookie);
        else {
            m_isConnected = true;
            emit connected();
        }
        return;
    }
    if (type == "error") {
        if (m_expectedReplies.empty()) {
            reportError(tr("An error was reported even though no request is open."));
            return;
        }
        const QString replyTo = data.value(IN_REPLY_TO_KEY).toString();
        const QVariant cookie = data.value(COOKIE_KEY);
        qCInfo(cmakeServerMode) << "Got \"error\" message." << replyTo << "(" << cookie << ")";

        const auto expected = m_expectedReplies.begin();
        if (expected->type != replyTo) {
            reportError(tr("Received an error in response to a request of type \"%1\", when a request of type \"%2\" was sent.")
                        .arg(replyTo).arg(expected->type));
            return;
        }
        if (expected->cookie != cookie) {
            reportError(tr("Received an error with cookie \"%1\", when \"%2\" was expected.")
                        .arg(cookie.toString()).arg(expected->cookie.toString()));
            return;
        }

        m_expectedReplies.erase(expected);

        emit cmakeError(data.value("errorMessage").toString(), replyTo, cookie);
        if (replyTo == HANDSHAKE_TYPE) {
            Core::Reaper::reap(m_cmakeProcess.release());
            m_cmakeSocket->disconnect();
            m_cmakeSocket->disconnectFromServer();
            m_cmakeSocket = nullptr;
            emit disconnected();
        }
        return;
    }
    if (type == "message") {
        const QString replyTo = data.value(IN_REPLY_TO_KEY).toString();
        const QVariant cookie = data.value(COOKIE_KEY);
        qCInfo(cmakeServerMode) << "Got \"message\" message." << replyTo << "(" << cookie << ")";

        const auto expected = m_expectedReplies.begin();
        if (expected->type != replyTo) {
            reportError(tr("Received a message in response to a request of type \"%1\", when a request of type \"%2\" was sent.")
                        .arg(replyTo).arg(expected->type));
            return;
        }
        if (expected->cookie != cookie) {
            reportError(tr("Received a message with cookie \"%1\", when \"%2\" was expected.")
                        .arg(cookie.toString()).arg(expected->cookie.toString()));
            return;
        }

        emit cmakeMessage(data.value("message").toString(), replyTo, cookie);
        return;
    }
    if (type == "progress") {
        const QString replyTo = data.value(IN_REPLY_TO_KEY).toString();
        const QVariant cookie = data.value(COOKIE_KEY);
        qCInfo(cmakeServerMode) << "Got \"progress\" message." << replyTo << "(" << cookie << ")";

        const auto expected = m_expectedReplies.begin();
        if (expected->type != replyTo) {
            reportError(tr("Received a progress report in response to a request of type \"%1\", when a request of type \"%2\" was sent.")
                        .arg(replyTo).arg(expected->type));
            return;
        }
        if (expected->cookie != cookie) {
            reportError(tr("Received a progress report with cookie \"%1\", when \"%2\" was expected.")
                        .arg(cookie.toString()).arg(expected->cookie.toString()));
            return;
        }

        emit cmakeProgress(data.value("progressMinimum").toInt(),
                           data.value("progressCurrent").toInt(),
                           data.value("progressMaximum").toInt(), replyTo, cookie);
        return;
    }
    if (type == "signal") {
        const QString replyTo = data.value(IN_REPLY_TO_KEY).toString();
        const QString cookie = data.value(COOKIE_KEY).toString();
        const QString name = data.value(NAME_KEY).toString();
        qCInfo(cmakeServerMode) << "Got \"signal\" message." << name << replyTo << "(" << cookie << ")";

        if (name.isEmpty()) {
            reportError(tr("Received a signal without a name."));
            return;
        }
        if (!replyTo.isEmpty() || isValid(cookie)) {
            reportError(tr("Received a signal in reply to a request."));
            return;
        }

        emit cmakeSignal(name, data);
        return;
    }
    reportError("Got a message of an unknown type.");
}

void ServerMode::handleHello(const QVariantMap &data)
{
    Q_UNUSED(data);
    QVariantMap extra;
    QVariantMap version;
    version.insert("major", m_majorProtocol);
    if (m_minorProtocol >= 0)
        version.insert("minor", m_minorProtocol);
    extra.insert("protocolVersion", version);
    extra.insert("sourceDirectory", m_sourceDirectory.toString());
    extra.insert("buildDirectory", m_buildDirectory.toString());
    extra.insert("generator", m_generator);
    if (!m_platform.isEmpty())
        extra.insert("platform", m_platform);
    if (!m_toolset.isEmpty())
        extra.insert("toolset", m_toolset);
    if (!m_extraGenerator.isEmpty())
        extra.insert("extraGenerator", m_extraGenerator);
    if (!m_platform.isEmpty())
        extra.insert("platform", m_platform);
    if (!m_toolset.isEmpty())
        extra.insert("toolset", m_toolset);

    sendRequest(HANDSHAKE_TYPE, extra);
}

void ServerMode::reportError(const QString &msg)
{
    qCWarning(cmakeServerMode) << "Report Error:" << msg;
    emit message(msg);
    emit errorOccured(msg);
}

} // namespace Internal
} // namespace CMakeProjectManager
