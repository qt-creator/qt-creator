/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Author: Andreas Hartmetz, KDAB (andreas.hartmetz@kdab.com)
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
    explicit ThreadedParser(QObject *parent = 0);
    ~ThreadedParser();

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
