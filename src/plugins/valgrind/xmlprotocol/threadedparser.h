// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>

QT_BEGIN_NAMESPACE
class QIODevice;
QT_END_NAMESPACE

namespace Valgrind {
namespace XmlProtocol {

class Error;
class Status;

/**
 * ThreadedParser for the Valgrind Output XmlProtocol 4
 */
class ThreadedParser : public QObject
{
    Q_OBJECT

public:
    explicit ThreadedParser(QObject *parent = nullptr);
    ~ThreadedParser() override;

    QString errorString() const;

    /// interface additions relative to Parser because Parser is synchronous and this
    /// class parses asynchronously in a non-public secondary thread.
    bool waitForFinished();
    bool isRunning() const;

    ///@warning will move @p stream to a different thread and take ownership of it
    void parse(QIODevice *stream);

private:
    void slotInternalError(const QString &errorString);

signals:
    void status(const Valgrind::XmlProtocol::Status &status);
    void error(const Valgrind::XmlProtocol::Error &error);
    void internalError(const QString &errorString);
    void errorCount(qint64 unique, qint64 count);
    void suppressionCount(const QString &name, qint64 count);
    void finished();

private:
    class Private;
    Private *const d;
};

} // XmlProtocol
} // Valgrind
