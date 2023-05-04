#include "layoutbuilder.h"

#include <QApplication>
#include <QDebug>

using namespace Layouting;

int main(int argc, char *argv[])
{
    Application app
    {
        resize(600, 400),
        title("Hello World"),

        Column {
            TextEdit {
                text("Hallo")
            },

            SpinBox {
                text("Quit"),
                onTextChanged([](const QString &text) { qDebug() << text; })
            },

            PushButton {
                text("Quit"),
                onClicked([] { QApplication::quit(); })
            },
        }
    };

    return app.exec(argc, argv);
}
