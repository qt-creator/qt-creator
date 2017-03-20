/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Author: Milian Wolff, KDAB (milian.wolff@kdab.com)
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

#include "../valgrindrunner.h"

namespace Valgrind {

namespace XmlProtocol{
class ThreadedParser;
}

namespace Memcheck {

class MemcheckRunner : public ValgrindRunner
{
    Q_OBJECT

public:
    explicit MemcheckRunner(QObject *parent = 0);
    ~MemcheckRunner();

    void setParser(XmlProtocol::ThreadedParser *parser);
    bool start();
    void disableXml();

signals:
    void logMessageReceived(const QByteArray &);

private:
    void localHostAddressRetrieved(const QHostAddress &localHostAddress);

    void xmlSocketConnected();
    void logSocketConnected();
    void readLogSocket();

    QString tool() const;

    bool startServers(const QHostAddress &localHostAddress);
    QStringList memcheckLogArguments() const;

    class Private;
    Private *d;
};

} // namespace Memcheck
} // namespace Valgrind
