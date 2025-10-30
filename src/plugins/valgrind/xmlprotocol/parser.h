// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/result.h>

#include <QtTaskTree/QTaskTree>

#include <QObject>

QT_BEGIN_NAMESPACE
class QAbstractSocket;
QT_END_NAMESPACE

namespace Valgrind::XmlProtocol {

class AnnounceThread;
class Error;
class ParserPrivate;
class Status;

/**
 * Parser for the Valgrind Output XML Protocol 4
 */
class Parser : public QObject
{
    Q_OBJECT

public:
    explicit Parser(QObject *parent = nullptr);
    ~Parser() override;

    QString errorString() const;
    // The passed device needs to be open. The parser takes ownership of the passed device.
    void setSocket(QAbstractSocket *socket);
    // Alternatively, the data to be parsed may be set manually
    void setData(const QByteArray &data);

    void start();
    bool isRunning() const;
    Utils::Result<> runBlocking();

signals:
    void status(const Status &status);
    void error(const Error &error);
    void errorCount(qint64 unique, qint64 count);
    void suppressionCount(const QString &name, qint64 count);
    void announceThread(const AnnounceThread &announceThread);
    void done(const Utils::Result<> &result);

private:
    std::unique_ptr<ParserPrivate> d;
};

class ParserTaskAdapter final
{
public:
    void operator()(Parser *task, QTaskInterface *iface)
    {
        QObject::connect(task, &Parser::done, iface, [iface](const Utils::Result<> &result) {
            iface->reportDone(QtTaskTree::toDoneResult(result == Utils::ResultOk));
        }, Qt::SingleShotConnection);
        task->start();
    }
};

using ParserTask = QCustomTask<Parser, ParserTaskAdapter>;

} // Valgrind::XmlProtocol
