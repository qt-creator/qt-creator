/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Author: Frank Osterfeld, KDAB (frank.osterfeld@kdab.com)
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#ifndef LIBVALGRIND_PROTOCOL_PARSER_H
#define LIBVALGRIND_PROTOCOL_PARSER_H

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

    explicit Parser(QObject *parent=0);
    ~Parser();

    QString errorString() const;

public Q_SLOTS:
    void parse(QIODevice *stream);

Q_SIGNALS:
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

#endif //LIBVALGRIND_PROTOCOL_PARSER_H
