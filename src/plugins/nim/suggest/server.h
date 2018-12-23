/****************************************************************************
**
** Copyright (C) Filippo Cucchetto <filippocucchetto@gmail.com>
** Contact: http://www.qt.io/licensing
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

#include <QDebug>
#include <QFile>
#include <QObject>
#include <QProcess>

namespace Nim {
namespace Suggest {

class NimSuggestServer : public QObject
{
    Q_OBJECT

public:
    NimSuggestServer(QObject *parent = nullptr);

    ~NimSuggestServer();

    bool start(const QString &executablePath, const QString &projectFilePath);

    void kill();

    quint16 port() const;

    QString executablePath() const;

    QString projectFilePath() const;

signals:
    void started();

    void finished();

    void crashed();

private:
    void onStarted();

    void onStandardOutputAvailable();

    void onFinished(int exitCode, QProcess::ExitStatus exitStatus);

    void clearState();

    bool m_started = false;
    bool m_portAvailable = false;
    QProcess m_process;
    quint16 m_port = 0;
    QString m_projectFilePath;
    QString m_executablePath;
};

} // namespace Suggest
} // namespace Nim
