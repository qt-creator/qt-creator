// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "lb.h"

#include <QApplication>
#include <QWidget>
#include <QLabel>
#include <QGroupBox>
#include <QTextEdit>
#include <QSpinBox>

using namespace Layouting;

void bindTest()
{
    QWidget *w = nullptr;
    QGroupBox *g = nullptr;
    QLabel *l = nullptr;

    Group {
        bindTo(&w),  // Works, as GroupInterface derives from WidgetInterface
        // bindTo(&l),  // Does (intentionally) not work, GroupInterface does not derive from LabelInterface
        bindTo(&g),
    };
}

// One source feeding two readers: editing the text edit updates both labels.
// The local Bindable goes out of scope on return, but its shared state stays
// alive: the created widgets hold it via their signal connections and notifiers.
Column direct()
{
    Bindable<QString> content;

    return Column {
        Label {
            text(content)
        },
        TextEdit {
            onValueChanged(content)
        },
        Label {
            text(content)
        },
    };
}

// A transformed binding: the spin box drives a text edit through a transform.
Column transformed()
{
    Bindable<int> spinBoxValue;

    Bindable<QString> spinBoxText = spinBoxValue.transformed<QString>([](int value) {
         return QString("World: %1").arg(value);
    });

    return Column {
        SpinBox {
            onValueChanged(spinBoxValue),
        },
        TextEdit {
            text(spinBoxText),
        },
    };
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    Group {
        size(300, 200),
        title("HHHHHHH"),
        Form {
            "Hallo",
            Group {
                title("Direct"),
                direct(),
            },
            Group {
                title("Transformed"),
                transformed(),
            },
            Group {
                title("Nested"),
                withBindable([](Bindable<int> &spinBoxValue) {
                    return Column {
                        TextEdit {
                            text(spinBoxValue.transformed<QString>([](int value) {
                                return QString("World: %1").arg(value);
                            })),
                        },
                        SpinBox {
                            onValueChanged(spinBoxValue),
                        },
                    };
                }),
            },
            br,
            "Grid",
            Grid {
                Span { 2,  Row { "1111111" } }, "3", br,
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
