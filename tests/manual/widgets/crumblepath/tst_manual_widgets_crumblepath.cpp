// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QApplication>
#include <QBoxLayout>
#include <QSettings>
#include <QTimer>
#include <QDebug>

#include <utils/crumblepath.h>
#include <utils/styledbar.h>

#include "../common/themeselector.h"

using namespace Utils;

QWidget *crumblePathWithMenu()
{
    auto *cp = new CrumblePath;
    cp->pushElement("Ene", QVariant(1));
    cp->addChild("Ene child 1", QVariant(1));
    cp->addChild("Ene child 2", QVariant(2));
    cp->pushElement("Mene", QVariant(2));
    cp->pushElement("Mopel", QVariant(3));
    cp->addChild("Mopel child 1", QVariant(1));
    cp->addChild("Mopel child 2", QVariant(2));
    return cp;
}

QWidget *disabledCrumblePathWithMenu()
{
    QWidget *cp = crumblePathWithMenu();
    cp->setDisabled(true);
    return cp;
}

QWidget *growingCrumblePath()
{
    auto *cp = new CrumblePath;
    auto *timer = new QTimer(cp);
    timer->start();
    QObject::connect(timer, &QTimer::timeout, cp, [cp, timer]() {
        const int elementId = cp->length() + 1;
        cp->pushElement(QStringLiteral("Element %1").arg(elementId), elementId);
        if (cp->length() == 5)
            timer->stop();
        else
            timer->setInterval(2000);
    });

    return cp;
}

QWidget *shrinkingCrumblePath()
{
    auto *cp = new CrumblePath;
    for (const auto &title : {"Ene", "Mene", "Mopel", "Zicke", "Zacke"})
        cp->pushElement(QLatin1String(title), title);

    auto *timer = new QTimer(cp);
    timer->setInterval(2000);
    timer->start();
    QObject::connect(timer, &QTimer::timeout, cp, [cp, timer](){
        cp->popElement();
        if (cp->length() == 1)
            timer->stop();
    });

    return cp;
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    auto *widget = new QWidget;
    auto *layout = new QVBoxLayout(widget);
    layout->addWidget(new ManualTest::ThemeSelector);
    for (auto creatorFunction : {crumblePathWithMenu,
                                 disabledCrumblePathWithMenu,
                                 growingCrumblePath,
                                 shrinkingCrumblePath}) {
        auto *cpToolBar = new Utils::StyledBar(widget);
        auto *cpLayout = new QHBoxLayout(cpToolBar);
        cpLayout->setContentsMargins(0, 0, 0, 0);
        cpLayout->addWidget(creatorFunction());
        layout->addWidget(cpToolBar);
    }
    layout->addStretch();

    widget->resize(600, 200);
    widget->show();

    return app.exec();
}
