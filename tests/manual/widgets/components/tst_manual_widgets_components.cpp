// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "../common/themeselector.h"

#include <QApplication>
#include <QTimer>

#include <utils/layoutbuilder.h>
#include <utils/qtcwidgets.h>

QWidget *widgets()
{
    using namespace Utils;

    auto widget = new QWidget;

    auto comboBox = new QtcComboBox;
    const QStringList content = QColor::colorNames();
    comboBox->addItems(content.first(8));

    auto switchOn = new QtcSwitch("Qt::RightToLeft");
    switchOn->setChecked(true);
    auto switchOff = new QtcSwitch("Qt::LeftToRight");
    switchOff->setLayoutDirection(Qt::LeftToRight);

    auto tabBar = new QtcTabBar;
    tabBar->addTab("Tab number 1");
    tabBar->addTab("2");
    tabBar->addTab("3");

    auto pageIndicator = new QtcPageIndicator;
    auto *pageTimer = new QTimer(widget);
    pageTimer->setInterval(1000);
    QObject::connect(pageTimer, &QTimer::timeout, pageIndicator, [pageIndicator]{
        const int nextPage = (pageIndicator->currentPage() + 1) % pageIndicator->pagesCount();
        pageIndicator->setCurrentPage(nextPage);
    });
    pageTimer->start();

    using namespace Layouting;
    Column {
        Group {
            title("Button"),
            Column {
                Row {
                    Column {
                        new QtcButton("LargePrimary", QtcButton::LargePrimary),
                        new QtcButton("SmallPrimary", QtcButton::SmallPrimary),
                    },
                    Column {
                        new QtcButton("LargeSecondary", QtcButton::LargeSecondary),
                        new QtcButton("SmallSecondary", QtcButton::SmallSecondary),
                    },
                    Column {
                        new QtcButton("LargeTertiary", QtcButton::LargeTertiary),
                        new QtcButton("SmallTertiary", QtcButton::SmallTertiary),
                    },
                    Column {
                        new QtcButton("LargeGhost", QtcButton::LargeGhost),
                        new QtcButton("SmallGhost", QtcButton::SmallGhost),
                    },
                },
                Row {
                    new QtcButton("SmallList", QtcButton::SmallList),
                    new QtcButton("SmallLink", QtcButton::SmallLink),
                    new QtcButton("Tag", QtcButton::Tag),
                },
            },
        },
        Group {
            title("Label"),
            Row {
                new QtcLabel("Primary", QtcLabel::Primary),
                new QtcLabel("Secondary", QtcLabel::Secondary),
            },
        },
        Row {
            Group {
                title("SearchBox"),
                Column {
                    new QtcSearchBox,
                },
            },
            Group {
                title("ComboBox"),
                Column {
                    comboBox,
                },
            },
        },
        Row {
            Group {
                title("Switch"),
                Column {
                    switchOn,
                    switchOff,
                },
            },
            Group {
                title("TabBar"),
                Row {
                    tabBar,
                },
            },
        },
        Group {
            title("PageIndicator"),
            Row {
                st, pageIndicator, st,
            },
        },
    }.attachTo(widget);

    return widget;
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    auto themeSelector = new ManualTest::ThemeSelector;

    QWidget *enabledWidgets = widgets();
    QWidget *disbledWidgets = widgets();
    disbledWidgets->setEnabled(false);

    QWidget widget;
    using namespace Layouting;
    Column {
        themeSelector,
        Row {enabledWidgets, disbledWidgets},
        st,
    }.attachTo(&widget);

    widget.show();

    return app.exec();
}
