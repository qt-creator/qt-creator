/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Author: Andreas Hartmetz, KDAB (andreas.hartmetz@kdab.com)
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef LIBVALGRIND_PROTOCOL_THREADEDPARSER_H
#define LIBVALGRIND_PROTOCOL_THREADEDPARSER_H

#include "../valgrind_global.h"

#include <QtCore/QObject>

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
class VALGRINDSHARED_EXPORT ThreadedParser : public QObject {
    Q_OBJECT
public:
    explicit ThreadedParser(QObject *parent=0);
    ~ThreadedParser();

    QString errorString() const;

    /// interface additions relative to Parser because Parser is synchronous and this
    /// class parses asynchronously in a non-public secondary thread.
    bool waitForFinished();
    bool isRunning() const;

public Q_SLOTS:
    ///@warning will move @p stream to a different thread and take ownership of it
    void parse(QIODevice *stream);

private Q_SLOTS:
    void slotInternalError(const QString &errorString);

Q_SIGNALS:
    void status(const Valgrind::XmlProtocol::Status &status);
    void error(const Valgrind::XmlProtocol::Error &error);
    void internalError(const QString &errorString);
    void errorCount(qint64 unique, qint64 count);
    void suppressionCount(const QString &name, qint64 count);
    void finished();

private:
    Q_DISABLE_COPY(ThreadedParser)

    class Private;
    Private *const d;
};

} // XmlProtocol
} // Valgrind

#endif //LIBVALGRIND_PROTOCOL_THREADEDPARSER_H
