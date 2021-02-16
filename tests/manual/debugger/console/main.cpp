/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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
