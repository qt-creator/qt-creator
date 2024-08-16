// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "../common/themeselector.h"

#include <QApplication>

#include <utils/layoutbuilder.h>

#include <coreplugin/welcomepagehelper.h>

QWidget *widgets()
{
    auto widget = new QWidget;

    auto comboBox = new Core::ComboBox;
    const QStringList content = QColor::colorNames();
    comboBox->addItems(content.first(8));

    using namespace Layouting;
    Column {
        Group {
            title("Core::Button"),
            Column {
                new Core::Button("MediumPrimary", Core::Button::MediumPrimary),
                new Core::Button("MediumSecondary", Core::Button::MediumSecondary),
                new Core::Button("SmallPrimary", Core::Button::SmallPrimary),
                new Core::Button("SmallSecondary", Core::Button::SmallSecondary),
                new Core::Button("SmallList", Core::Button::SmallList),
                new Core::Button("SmallLink", Core::Button::SmallLink),
                new Core::Button("Tag", Core::Button::Tag),
            },
        },
        Group {
            title("Core::Label"),
            Column {
                new Core::Label("Primary", Core::Label::Primary),
                new Core::Label("Secondary", Core::Label::Secondary),
            },
        },
        Group {
            title("Core::SearchBox"),
            Column {
                new Core::SearchBox,
            },
        },
        Group {
            title("Core::ComboBox"),
            Column {
                comboBox,
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
