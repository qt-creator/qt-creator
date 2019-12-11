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

#include <QApplication>
#include <QBoxLayout>
#include <QSettings>
#include <QTimer>
#include <QDebug>

#include <utils/crumblepath.h>
#include <utils/styledbar.h>
#include <utils/stylehelper.h>
#include <utils/theme/theme.h>
#include <utils/theme/theme_p.h>

#include <coreplugin/manhattanstyle.h>

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
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

    QApplication app(argc, argv);

    Theme theme("");
    QSettings settings(":/themes/flat.creatortheme", QSettings::IniFormat);
    theme.readSettings(settings);
    setCreatorTheme(&theme);
    StyleHelper::setBaseColor(QColor(StyleHelper::DEFAULT_BASE_COLOR));
    QApplication::setStyle(new ManhattanStyle(creatorTheme()->preferredStyles().value(0)));

    auto *widget = new QWidget;
    auto *layout = new QVBoxLayout(widget);
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
