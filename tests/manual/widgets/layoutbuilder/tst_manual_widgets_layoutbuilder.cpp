// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <utils/layoutbuilder.h>

#include <QApplication>
#include <QLineEdit>
#include <QTextEdit>

using namespace Layouting;

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    auto lineEdit = new QLineEdit("0");

    auto minusClick = [lineEdit] {
        lineEdit->setText(QString::number(lineEdit->text().toInt() + 1));
    };

    auto plusClick = [lineEdit] {
        lineEdit->setText(QString::number(lineEdit->text().toInt() + 1));
    };

    Row {
        PushButton { text("-"), onClicked(minusClick) },
        lineEdit,
        PushButton { text("+"), onClicked(plusClick) },
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
        Column {"First Widget"},
        Row {"Second Widget"},
    }.emerge()->show();

    return app.exec();
}
