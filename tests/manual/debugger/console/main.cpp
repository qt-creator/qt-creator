// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QCoreApplication>
#include <QDebug>
#include <QProcess>
#include <QThread>

#include <iostream>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    // Checks for "Run as root"
    QProcess proc;
    proc.start("id", QStringList());
    proc.waitForFinished();
    qDebug() << proc.readAllStandardOutput();

    // Checks terminal input
    std::cout << "Enter some random data to check terminal input:" << std::endl;
    std::string res;
    std::getline(std::cin, res);

    // Check for interruptability
    std::cout << "Press Ctrl-C or the interrupt button in the debugger.\n";
    while (true) {
        ++argc;
        ++argc;
        std::cerr << argc << std::endl;
        QThread::sleep(1);
        ++argc;
        ++argc;
    }

    return 0;
}
