// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "argumentscollector.h"
#include "../dpastedotcomprotocol.h"
#include "../pastebindotcomprotocol.h"

#include <QFile>
#include <QObject>
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
        if (protocol == PasteBinDotComProtocol::protocolName().toLower())
            m_protocol.reset(new PasteBinDotComProtocol);
        else if (protocol == DPasteDotComProtocol::protocolName().toLower())
            m_protocol.reset(new DPasteDotComProtocol);
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
            QCoreApplication::exit(EXIT_FAILURE);
            return;
        }
        const QString content = QString::fromLocal8Bit(file.readAll());
        if (content.isEmpty()) {
            std::cerr << "Empty input, aborting." << std::endl;
            QCoreApplication::exit(EXIT_FAILURE);
            return;
        }
        connect(m_protocol.data(), &Protocol::pasteDone, this, &PasteReceiver::handlePasteDone);
        m_protocol->paste(content);
    }

private:
    void handlePasteDone(const QString &link)
    {
        std::cout << qPrintable(link) << std::endl;
        QCoreApplication::quit();
    }

    const QString m_filePath;
    QScopedPointer<Protocol> m_protocol;
};

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    const QStringList protocols = {DPasteDotComProtocol::protocolName().toLower(),
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
        for (const QString &protocol : protocols)
            std::cout << qPrintable(protocol) << std::endl;
        return EXIT_SUCCESS;
    case ArgumentsCollector::RequestTypePaste: {
        PasteReceiver pr(argsCollector.protocol(), argsCollector.inputFilePath());
        QMetaObject::invokeMethod(&pr, &PasteReceiver::paste, Qt::QueuedConnection);
        return app.exec();
    }
    }
}

#include "main.moc"
