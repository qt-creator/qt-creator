#pragma once

#include "layoutbuilder.h"

#include <QCoreApplication>
#include <QWidget>

using namespace Layouting;

class ApplicationWindow : public QWidget
{
public:
    ApplicationWindow()
    {
        resize(600, 400);
        setWindowTitle("Hello World");

        Column {
            TextEdit {
                text("Hallo")
            },

            PushButton {
                text("Quit"),
                onClicked([] { QCoreApplication::quit(); })
            }
        }.attachTo(this);
    }
};
