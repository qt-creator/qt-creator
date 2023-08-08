// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>
#include <QPointer>

QT_BEGIN_NAMESPACE
class QIODevice;
QT_END_NAMESPACE

namespace Valgrind::XmlProtocol {

class Error;
class Status;
class Thread;

/**
 * ThreadedParser for the Valgrind Output XmlProtocol 4
 */
class ThreadedParser : public QObject
{
    Q_OBJECT

public:
    explicit ThreadedParser(QObject *parent = nullptr);

    /// interface additions relative to Parser because Parser is synchronous and this
    /// class parses asynchronously in a non-public secondary thread.
    bool waitForFinished();
    bool isRunning() const;

    ///@warning will move @p stream to a different thread and take ownership of it
    void parse(QIODevice *stream);

signals:
    void status(const Status &status);
    void error(const Error &error);
    void done(bool success, const QString &errorString);

private:
    QPointer<Thread> m_parserThread;
};

} // Valgrind::XmlProtocol
