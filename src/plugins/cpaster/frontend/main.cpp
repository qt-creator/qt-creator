/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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
        if (protocol == KdePasteProtocol::protocolName().toLower())
            m_protocol.reset(new KdePasteProtocol);
        else if (protocol == PasteBinDotCaProtocol::protocolName().toLower())
            m_protocol.reset(new PasteBinDotCaProtocol);
        else if (protocol == PasteBinDotComProtocol::protocolName().toLower())
            m_protocol.reset(new PasteBinDotComProtocol);
        else
            qFatal("Internal error: Invalid protocol.");
    }

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
        connect(m_protocol.data(), &Protocol::pasteDone, this, &PasteReceiver::handlePasteDone);
        m_protocol->paste(content);
    }

private:
    void handlePasteDone(const QString &link)
    {
        std::cout << qPrintable(link) << std::endl;
        qApp->quit();
    }

    const QString m_filePath;
    QScopedPointer<Protocol> m_protocol;
};

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    const QStringList protocols = {KdePasteProtocol::protocolName().toLower(),
                                   PasteBinDotCaProtocol::protocolName().toLower(),
                                   PasteBinDotComProtocol::protocolName().toLower()};
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
        QTimer::singleShot(0, &pr, &PasteReceiver::paste);
        return app.exec();
    }
    }
}

#include "main.moc"
