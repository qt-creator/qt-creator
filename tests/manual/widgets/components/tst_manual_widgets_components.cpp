// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "../common/themeselector.h"

#include <QApplication>

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

    using namespace Layouting;
    Column {
        Group {
            title("Button"),
            Column {
                new QtcButton("LargePrimary", QtcButton::LargePrimary),
                new QtcButton("LargeSecondary", QtcButton::LargeSecondary),
                new QtcButton("LargeTertiary", QtcButton::LargeTertiary),
                new QtcButton("SmallPrimary", QtcButton::SmallPrimary),
                new QtcButton("SmallSecondary", QtcButton::SmallSecondary),
                new QtcButton("SmallTertiary", QtcButton::SmallTertiary),
                new QtcButton("SmallList", QtcButton::SmallList),
                new QtcButton("SmallLink", QtcButton::SmallLink),
                new QtcButton("Tag", QtcButton::Tag),
            },
        },
        Group {
            title("Label"),
            Column {
                new QtcLabel("Primary", QtcLabel::Primary),
                new QtcLabel("Secondary", QtcLabel::Secondary),
            },
        },
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
        Group {
            title("Switch"),
            Column {
                switchOn,
                switchOff,
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
