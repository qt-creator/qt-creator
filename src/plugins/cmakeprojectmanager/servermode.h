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

#pragma once

#include <utils/qtcprocess.h>

#include <QLoggingCategory>
#include <QTimer>
#include <QVariantMap>
#include <QTemporaryDir>

#include <memory>

QT_FORWARD_DECLARE_CLASS(QLocalSocket);

namespace Utils { class QtcProcess; }

namespace CMakeProjectManager {
namespace Internal {

class ServerMode : public QObject
{
    Q_OBJECT

public:
    ServerMode(const Utils::Environment &env,
               const Utils::FileName &sourceDirectory, const Utils::FileName &buildDirectory,
               const Utils::FileName &cmakeExecutable,
               const QString &generator, const QString &extraGenerator,
               const QString &platform, const QString &toolset,
               bool experimental, int major, int minor = -1,
               QObject *parent = nullptr);
    ~ServerMode() final;

    void sendRequest(const QString &type, const QVariantMap &extra = QVariantMap(),
                     const QVariant &cookie = QVariant());

    bool isConnected();

signals:
    void connected();
    void disconnected();
    void message(const QString &msg);
    void errorOccured(const QString &msg);

    // Forward stuff from the server
    void cmakeReply(const QVariantMap &data, const QString &inResponseTo, const QVariant &cookie);
    void cmakeError(const QString &errorMessage, const QString &inResponseTo, const QVariant &cookie);
    void cmakeMessage(const QString &message, const QString &inResponseTo, const QVariant &cookie);
    void cmakeProgress(int min, int cur, int max, const QString &inResponseTo, const QVariant &cookie);
    void cmakeSignal(const QString &name, const QVariantMap &data);

private:
    void connectToServer();
    void handleCMakeFinished(int code, QProcess::ExitStatus status);

    void handleRawCMakeServerData();
    void parseBuffer(const QByteArray &buffer);
    void parseJson(const QVariantMap &data);

    void handleHello(const QVariantMap &data);

    void reportError(const QString &msg);

#if defined(Q_OS_UNIX)
    QTemporaryDir m_socketDir;
#endif
    std::unique_ptr<Utils::QtcProcess> m_cmakeProcess;
    QLocalSocket *m_cmakeSocket = nullptr;
    QTimer m_connectionTimer;

    Utils::FileName m_sourceDirectory;
    Utils::FileName m_buildDirectory;
    Utils::FileName m_cmakeExecutable;

    QByteArray m_buffer;

    struct ExpectedReply {
        QString type;
        QVariant cookie;
    };
    std::vector<ExpectedReply> m_expectedReplies;

    const QString m_generator;
    const QString m_extraGenerator;
    const QString m_platform;
    const QString m_toolset;
    QString m_socketName;
    const bool m_useExperimental;
    bool m_gotHello = false;
    bool m_isConnected = false;
    const int m_majorProtocol = -1;
    const int m_minorProtocol = -1;

    int m_requestCounter = 0;

};

} // namespace Internal
} // namespace CMakeProjectManager

Q_DECLARE_LOGGING_CATEGORY(cmakeServerMode);
