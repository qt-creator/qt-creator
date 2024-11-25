// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "lb.h"

#include <QApplication>
#include <QWidget>
#include <QLabel>
#include <QGroupBox>
#include <QTextEdit>

using namespace Layouting;

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    TextEdit::Id textId;

    QWidget *w = nullptr;
    QGroupBox *g = nullptr;
    QLabel *l = nullptr;

    Group {
        bindTo(&w),  // Works, as GroupInterface derives from WidgetInterface
        // bindTo(&l),  // Does (intentionally) not work, GroupInterface does not derive from LabelInterface
        bindTo(&g),
        size(300, 200),
        title("HHHHHHH"),
        Form {
            "Hallo",
            Group {
                title("Title"),
                Column {
                    Label {
                        text("World")
                    },
                    TextEdit {
                        id(&textId),
                        text("Och noe")
                    }
                }
            },
            br,
            "Col",
            Column {
                Row { "1", "2", "3" },
                Row { "3", "4", "6" }
            },
            br,
            "Grid",
            Grid {
                Span { 2, QString("1111111") }, "3", br,
                "3", "4", "6", br,
                "4", empty, "6", br,
                hr, "4", "6"
            },
            br,
            Column {
                Label {
                    text("Hi"),
                    size(30, 20)
                },
                Row {
                    SpinBox {
                        onTextChanged([&](const QString &text) { textId->setText(text); })
                    },
                    st,
                    PushButton {
                        text("Quit"),
                        onClicked(QApplication::quit, &app)
                    }
                }
            }
        }
    }.show();

    return app.exec();
}
