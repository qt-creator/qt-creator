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

#include <QCoreApplication>
#include <QDebug>
#include <QString>
#include <iostream>
#include <string>

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    QString qs("I'm a QString");
    std::string stds("I'm a std::string");
    char c[] = "I'm a char c[]";
    qDebug() << "This is a QString:" << qs;
    std::cout << "This is a std::string: " << stds << std::endl;
    std::cout << "This is a char c[]: " << c << std::endl;

    qDebug() << "This is QDebug";
    std::cout << "This is stdout" << std::endl;
    std::cerr << "This is stderr" << std::endl;

    return 0;   // Comment out for testing stdin

    std::string s;
    std::cout << "Enter something: ";
    std::cin >> s;
    std::cout << "You entered: " << s << std::endl;
    return 0;
}
