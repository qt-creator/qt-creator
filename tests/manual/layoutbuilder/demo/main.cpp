// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "layoutbuilder.h"

#include <QApplication>

using namespace Layouting;

int main(int argc, char *argv[])
{
    ID textId;

    Application app
    {
        resize(600, 400),
        title("Hello World"),

        Column {
            If { false, {
                    TextEdit {
                        id(textId),
                        text("Hallo")
                    },
                }, {
                    TextEdit {
                        id(textId),
                        text("Och noe")
                    },
                }
            },

            Row {
                SpinBox {
                    onTextChanged([&](const QString &text) { setText(textId, text); })
                },

                Stretch(),

                PushButton {
                    text("Quit"),
                    onClicked(QApplication::quit)
                },
             }
        }
    };

    return app.exec(argc, argv);
}
