/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
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
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#ifndef OUTPUT_COLLECTOR_H
#define OUTPUT_COLLECTOR_H

#include <QtCore/QByteArray>
#include <QtCore/QObject>

QT_BEGIN_NAMESPACE
class QLocalServer;
class QLocalSocket;
class QSocketNotifier;
QT_END_NAMESPACE

namespace Debugger {
namespace Internal {

///////////////////////////////////////////////////////////////////////
//
// OutputCollector
//
///////////////////////////////////////////////////////////////////////

class OutputCollector : public QObject
{
    Q_OBJECT

public:
    OutputCollector(QObject *parent = 0);
    ~OutputCollector();
    bool listen();
    void shutdown();
    QString serverName() const;
    QString errorString() const;

signals:
    void byteDelivery(const QByteArray &data);

private slots:
    void bytesAvailable();
#ifdef Q_OS_WIN
    void newConnectionAvailable();
#endif

private:
#ifdef Q_OS_WIN
    QLocalServer *m_server;
    QLocalSocket *m_socket;
#else
    QString m_serverPath;
    int m_serverFd;
    QSocketNotifier *m_serverNotifier;
    QString m_errorString;
#endif
};

} // namespace Internal
} // namespace Debugger

#endif // OUTPUT_COLLECTOR_H
