// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
    int i = 1892;
    qDebug() << "This is a QString:" << qs;
    std::cout << "This is a std::string: " << stds << std::endl;
    std::cout << "This is a char c[]: " << c << std::endl;
    std::cout << "This is an int: " << i << std::endl;

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
