// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "../common/themeselector.h"

#include <utils/layoutbuilder.h>
#include <utils/theme/theme.h>
#include <utils/theme/theme_p.h>

#include <coreplugin/welcomepagehelper.h>

#include <QApplication>
#include <QLineEdit>
#include <QStyle>
#include <QTextEdit>
#include <QToolButton>

using namespace Layouting;
using CoreButton = Core::CoreButton;

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    auto lineEdit = new QLineEdit("0");

    auto minusClick = [lineEdit] {
        lineEdit->setText(QString::number(lineEdit->text().toInt() - 1));
    };

    auto plusClick = [lineEdit] {
        lineEdit->setText(QString::number(lineEdit->text().toInt() + 1));
    };

    // clang-format off
    Row {
        PushButton { text("-"), onClicked(qApp, minusClick) },
        lineEdit,
        PushButton { text("+"), onClicked(qApp, plusClick) },
        Group {
            title("Splitter in Group"),
            Column {
                Splitter {
                    new QTextEdit("First Widget"),
                    new QTextEdit("Second Widget"),
                },
            }
        },
    }.emerge()->show();

    Group {
        windowTitle("Group without parent layout"),
        title("This group was emerged without parent layout"),
        Column {
            Splitter {
                new QTextEdit("First Widget"),
                new QTextEdit("Second Widget"),
            },
        }
    }.emerge()->show();


    Splitter {
        windowTitle("Splitter with sub layouts"),
        Column { QString("First Widget") },
        Row { QString("Second Widget") },
    }.emerge()->show();

    QToolButton *toolButton1;
    QToolButton *toolButton2;
    QToolButton *toolButton3;
    QWidget *flowlayouts = Column {
        Label { wordWrap(true), text("All push buttons:") },
        Flow {
            PushButton { text("button1") },
            PushButton { text("button2") },
            PushButton { text("button3") },
            PushButton { text("button4") },
            PushButton { text("button5") }
        },
        hr,
        Label { wordWrap(true), text("Mixed buttons, that can have different spacing:") },
        Flow {
            PushButton { text("a pushbutton") },
            ToolButton { bindTo(&toolButton1) },
            ToolButton { bindTo(&toolButton2) },
            ToolButton { bindTo(&toolButton3) },
        },
        Label { wordWrap(true), text("Right aligned:") },
        Flow {
            alignment(Qt::AlignRight),
            PushButton { text("button1") },
            PushButton { text("button2") },
            PushButton { text("button3") },
            PushButton { text("button4") },
            PushButton { text("button5") }
        },
        Label { wordWrap(true), text("Spacing:") },
        Flow {
            spacing(1),
            PushButton { text("button1") },
            PushButton { text("button2") },
            PushButton { text("button3") },
            PushButton { text("button4") }
        },
        Label { text("Core Button:") },
        new ManualTest::ThemeSelector,
        Flow {
            CoreButton {
                text("Large Primary"),
                role(Core::Button::Role::LargePrimary)
            },
            CoreButton {
                text("Large Secondary"),
                role(Core::Button::Role::LargeSecondary)
            },
            CoreButton {
                text("Large Tertiary"),
                role(Core::Button::Role::LargeTertiary)
            },
            CoreButton {
                text("Small Primary"),
                role(Core::Button::Role::SmallPrimary)
            },
            CoreButton {
                text("Small Secondary"),
                role(Core::Button::Role::SmallSecondary)
            },
            CoreButton {
                text("Small Tertiary"),
                role(Core::Button::Role::SmallTertiary)
            },
            CoreButton {
                text("Small List"),
                role(Core::Button::Role::SmallList)
            },
            CoreButton {
                text("Small Link"),
                role(Core::Button::Role::SmallLink)
            },
            CoreButton {
                text("Tag"),
                role(Core::Button::Role::Tag)
            },
        },
        st
    }.emerge();
    toolButton1->setDefaultAction(new QAction("tool button 1", toolButton1));
    toolButton2->setDefaultAction(new QAction("tool button 2", toolButton2));
    toolButton3->setDefaultAction(new QAction(
        qApp->style()->standardIcon(QStyle::SP_TitleBarCloseButton), "", toolButton2));
    flowlayouts->setWindowTitle("Flow Layouts");
    flowlayouts->adjustSize();
    flowlayouts->show();
    // clang-format on
    return app.exec();
}
