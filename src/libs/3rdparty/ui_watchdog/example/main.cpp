/*
  This file is part of KDToolBox.

  SPDX-FileCopyrightText: 2017 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.com>
  Author: Sérgio Martins <sergio.martins@kdab.com>

  SPDX-License-Identifier: MIT
*/

#include "uiwatchdog.h"

#include <QDebug>
#include <QTimer>
#include <QtWidgets>

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    QWidget w;
    auto layout = new QVBoxLayout(&w);
    auto button = new QPushButton(QStringLiteral("Block UI completely"));
    auto button2 = new QPushButton(QStringLiteral("Sleep every other second"));
    auto button3 = new QPushButton(QStringLiteral("Cancel sleep"));
    layout->addWidget(button);
    layout->addWidget(button2);
    layout->addStretch();
    layout->addWidget(button3);

    QObject::connect(button, &QPushButton::clicked, [] {
        qDebug() << "Blocking forever!";
        while (true)
            ;
    });

    auto sleepTimer = new QTimer(&w);
    sleepTimer->setInterval(1000);
    QObject::connect(sleepTimer, &QTimer::timeout, [] {
        qDebug() << "Sleeping";
        QThread::sleep(1);
        qDebug() << "Waking up";
    });

    QObject::connect(button2, &QPushButton::clicked, sleepTimer, QOverload<>::of(&QTimer::start));
    QObject::connect(button3, &QPushButton::clicked, sleepTimer, &QTimer::stop);

    UiWatchdog dog;
    dog.start();
    w.resize(800, 800);
    w.show();

    return app.exec();
}
