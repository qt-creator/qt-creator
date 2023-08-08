// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>

QT_BEGIN_NAMESPACE
class QIODevice;
QT_END_NAMESPACE

namespace Valgrind::XmlProtocol {

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
    explicit Parser(QObject *parent = nullptr);
    ~Parser() override;

    QString errorString() const;
    // The passed device needs to be open. The parser takes ownership of the passed device.
    void setIODevice(QIODevice *device);
    void start();

signals:
    void status(const Status &status);
    void error(const Error &error);
    void errorCount(qint64 unique, qint64 count);
    void suppressionCount(const QString &name, qint64 count);
    void announceThread(const AnnounceThread &announceThread);
    void done(bool success, const QString &errorString);

private:
    class Private;
    Private *const d;
};

} // Valgrind::XmlProtocol
