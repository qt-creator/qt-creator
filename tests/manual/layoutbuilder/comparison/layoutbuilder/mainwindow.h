#pragma once

#include "layoutbuilder.h"

#include <QApplication>

using namespace Layouting;

Application app
{
    resize(600, 400),
    title("Hello World"),

    Column {
        TextEdit {
            text("Hallo")
        },

        PushButton {
            text("Quit"),
            onClicked(QApplication::quit)
        }
    }
};
