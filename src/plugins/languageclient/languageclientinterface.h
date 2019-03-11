/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include <languageserverprotocol/basemessage.h>

#include <QBuffer>
#include <QProcess>

namespace LanguageClient {

class StdIOSettings;

class BaseClientInterface : public QObject
{
    Q_OBJECT
public:
    BaseClientInterface();

    virtual ~BaseClientInterface();

    void sendMessage(const LanguageServerProtocol::BaseMessage &message);
    virtual bool start() { return true; }

    void resetBuffer();

signals:
    void messageReceived(LanguageServerProtocol::BaseMessage message);
    void finished();
    void error(const QString &message);

protected:
    virtual void sendData(const QByteArray &data) = 0;
    void parseData(const QByteArray &data);

private:
    QBuffer m_buffer;
    LanguageServerProtocol::BaseMessage m_currentMessage;
};

class StdIOClientInterface : public BaseClientInterface
{
    Q_OBJECT
public:
    StdIOClientInterface(const QString &executable, const QString &arguments);
    ~StdIOClientInterface() override;

    StdIOClientInterface() = delete;
    StdIOClientInterface(const StdIOClientInterface &) = delete;
    StdIOClientInterface(StdIOClientInterface &&) = delete;
    StdIOClientInterface &operator=(const StdIOClientInterface &) = delete;
    StdIOClientInterface &operator=(StdIOClientInterface &&) = delete;

    bool needsRestart(const StdIOSettings *settings) const;

    bool start() override;

    void setWorkingDirectory(const QString &workingDirectory);

protected:
    void sendData(const QByteArray &data) final;
    QProcess m_process;

private:
    void readError();
    void readOutput();
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);

    const QString m_executable;
    const QString m_arguments;
};

} // namespace LanguageClient
