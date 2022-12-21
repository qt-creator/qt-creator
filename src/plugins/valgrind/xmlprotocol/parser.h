// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>

QT_BEGIN_NAMESPACE
class QIODevice;
QT_END_NAMESPACE

namespace Valgrind {
namespace XmlProtocol {

class AnnounceThread;
class Error;
class Status;

/**
 * Parser for the Valgrind Output XML Protocol 4
 */
class Parser : public QObject
{
    Q_OBJECT

public:
    enum Tool {
        Unknown,
        Memcheck,
        Ptrcheck,
        Helgrind
    };

    explicit Parser(QObject *parent = nullptr);
    ~Parser() override;

    QString errorString() const;
    void parse(QIODevice *stream);

signals:
    void status(const Valgrind::XmlProtocol::Status &status);
    void error(const Valgrind::XmlProtocol::Error &error);
    void internalError(const QString &errorString);
    void errorCount(qint64 unique, qint64 count);
    void suppressionCount(const QString &name, qint64 count);
    void announceThread(const Valgrind::XmlProtocol::AnnounceThread &announceThread);
    void finished();

private:
    class Private;
    Private *const d;
};

} // XmlProtocol
} // Valgrind
