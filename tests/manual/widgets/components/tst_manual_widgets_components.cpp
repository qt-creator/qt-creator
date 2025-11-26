// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "../common/themeselector.h"

#include <QApplication>
#include <QMetaEnum>
#include <QTimer>

#include <utils/icon.h>
#include <utils/layoutbuilder.h>
#include <utils/qtcwidgets.h>

using namespace Utils;

static QWidget *button(QtcButton::Role role, bool withPixmap = false)
{
    static const int roleEnumIndex = QtcButton::staticMetaObject.indexOfEnumerator("Role");
    static const QMetaEnum roleEnum = QtcButton::staticMetaObject.enumerator(roleEnumIndex);
    auto button = new QtcButton(roleEnum.key(role), role);
    button->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
    if (withPixmap) {
        static const QPixmap pixmap =
            Icon({{":/utils/images/zoom.png", Theme::Token_Text_On_Accent}}, Icon::Tint).pixmap();
        button->setPixmap(pixmap);
    }
    return button;
}

QWidget *widgets()
{
    auto widget = new QWidget;

    auto comboBox = new QtcComboBox;
    const QStringList content = QColor::colorNames();
    comboBox->addItems(content.first(8));

    auto switchOn = new QtcSwitch("Qt::RightToLeft");
    switchOn->setChecked(true);
    auto switchOff = new QtcSwitch("Qt::LeftToRight");
    switchOff->setLayoutDirection(Qt::LeftToRight);

    auto tabBar = new QtcTabBar;
    tabBar->setExpanding(false);
    tabBar->setMovable(true);
    tabBar->setTabsClosable(true);
    tabBar->addTab("Tab 1");
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
                        button(QtcButton::LargePrimary),
                        button(QtcButton::MediumPrimary),
                        button(QtcButton::SmallPrimary),
                    },
                    Column {
                        button(QtcButton::LargeSecondary),
                        button(QtcButton::MediumSecondary),
                        button(QtcButton::SmallSecondary),
                    },
                    Column {
                        button(QtcButton::LargeTertiary),
                        button(QtcButton::MediumTertiary),
                        button(QtcButton::SmallTertiary),
                    },
                    Column {
                        button(QtcButton::LargeGhost),
                        button(QtcButton::MediumGhost),
                        button(QtcButton::SmallGhost),
                    },
                    st,
                },
                Flow {
                    button(QtcButton::SmallList),
                    button(QtcButton::SmallLink),
                    button(QtcButton::Tag),
                },
            },
        },
        Group {
            title("Button with Pixmap"),
            Column {
                Flow {
                    button(QtcButton::LargePrimary, true),
                    button(QtcButton::MediumPrimary, true),
                    button(QtcButton::SmallPrimary, true),
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
