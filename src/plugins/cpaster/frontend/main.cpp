/**************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "argumentscollector.h"
#include "../kdepasteprotocol.h"
#include "../pastebindotcaprotocol.h"
#include "../pastebindotcomprotocol.h"

#include <QFile>
#include <QObject>
#include <QTimer>
#include <QCoreApplication>

#include <cstdio>
#include <cstdlib>
#include <iostream>

using namespace CodePaster;

class PasteReceiver : public QObject
{
    Q_OBJECT
public:
    PasteReceiver(const QString &protocol, const QString &filePath) : m_filePath(filePath)
    {
        const QSharedPointer<NetworkAccessManagerProxy> accessMgr(new NetworkAccessManagerProxy);
        if (protocol == KdePasteProtocol::protocolName().toLower())
            m_protocol.reset(new KdePasteProtocol(accessMgr));
        else if (protocol == PasteBinDotCaProtocol::protocolName().toLower())
            m_protocol.reset(new PasteBinDotCaProtocol(accessMgr));
        else if (protocol == PasteBinDotComProtocol::protocolName().toLower())
            m_protocol.reset(new PasteBinDotComProtocol(accessMgr));
        else
            qFatal("Internal error: Invalid protocol.");
    }

public slots:
    void paste()
    {
        QFile file(m_filePath);
        const bool success = m_filePath.isEmpty()
                ? file.open(stdin, QIODevice::ReadOnly) : file.open(QIODevice::ReadOnly);
        if (!success) {
            std::cerr << "Error: Failed to open file to paste from." << std::endl;
            qApp->exit(EXIT_FAILURE);
            return;
        }
        const QString content = QString::fromLocal8Bit(file.readAll());
        if (content.isEmpty()) {
            std::cerr << "Empty input, aborting." << std::endl;
            qApp->exit(EXIT_FAILURE);
            return;
        }
        connect(m_protocol.data(), SIGNAL(pasteDone(QString)), SLOT(handlePasteDone(QString)));
        m_protocol->paste(content);
    }

private slots:
    void handlePasteDone(const QString &link)
    {
        std::cout << qPrintable(link) << std::endl;
        qApp->quit();
    }

private:
    const QString m_filePath;
    QScopedPointer<Protocol> m_protocol;
};

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    const QStringList protocols = QStringList() << KdePasteProtocol::protocolName().toLower()
            << PasteBinDotCaProtocol::protocolName().toLower()
            << PasteBinDotComProtocol::protocolName().toLower();
    ArgumentsCollector argsCollector(protocols);
    QStringList arguments = QCoreApplication::arguments();
    arguments.removeFirst();
    if (!argsCollector.collect(arguments)) {
        std::cerr << "Error: " << qPrintable(argsCollector.errorString()) << '.' << std::endl
                  << qPrintable(argsCollector.usageString()) << std::endl;
        return EXIT_FAILURE;
    }

    switch (argsCollector.requestType()) {
    case ArgumentsCollector::RequestTypeHelp:
        std::cout << qPrintable(argsCollector.usageString()) << std::endl;
        return EXIT_SUCCESS;
    case ArgumentsCollector::RequestTypeListProtocols:
        foreach (const QString &protocol, protocols)
            std::cout << qPrintable(protocol) << std::endl;
        return EXIT_SUCCESS;
    case ArgumentsCollector::RequestTypePaste: {
        PasteReceiver pr(argsCollector.protocol(), argsCollector.inputFilePath());
        QTimer::singleShot(0, &pr, SLOT(paste()));
        return app.exec();
    }
    }
}

#include "main.moc"
