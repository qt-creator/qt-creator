// Copyright (C) Filippo Cucchetto <filippocucchetto@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QDebug>
#include <QFile>
#include <QObject>

#include <utils/qtcprocess.h>

namespace Nim {
namespace Suggest {

class NimSuggestServer : public QObject
{
    Q_OBJECT

public:
    NimSuggestServer(QObject *parent = nullptr);

    bool start(const QString &executablePath, const QString &projectFilePath);
    void stop();

    quint16 port() const;
    QString executablePath() const;
    QString projectFilePath() const;

signals:
    void started();
    void done();

private:
    void onStandardOutputAvailable();
    void onDone();
    void clearState();

    bool m_portAvailable = false;
    Utils::QtcProcess m_process;
    quint16 m_port = 0;
    QString m_projectFilePath;
    QString m_executablePath;
};

} // namespace Suggest
} // namespace Nim
