
#include "handler.h"

#include <QtCore/QDebug>

#include <QtGui/QApplication>
#include <QtGui/QMainWindow>
#include <QtGui/QPlainTextEdit>
#include <QtGui/QStatusBar>
#include <QtGui/QTextEdit>

using namespace FakeVim::Internal;

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QStringList args = app.arguments();
    (void) args.takeFirst();

    QWidget *widget = 0;
    QString title;
    bool usePlainTextEdit = args.size() < 2;
    if (usePlainTextEdit) {
        widget = new QPlainTextEdit;
        title = "PlainTextEdit";
    } else {
        widget = new QTextEdit;
        title = "TextEdit";
    }
    widget->resize(450, 350);
    widget->setFocus();


    FakeVimHandler fakeVim;

    QMainWindow mw;
    mw.setWindowTitle("Fakevim (" + title + ")");
    mw.setCentralWidget(widget);
    mw.resize(500, 650);
    mw.move(0, 0);
    mw.show();
    
    QFont font = widget->font();
    //: -misc-fixed-medium-r-semicondensed--13-120-75-75-c-60-iso8859-1
    //font.setFamily("Misc");
    font.setFamily("Monospace");
    //font.setStretch(QFont::SemiCondensed);

    widget->setFont(font);
    mw.statusBar()->setFont(font);

    QObject::connect(&fakeVim, SIGNAL(commandBufferChanged(QString)),
        mw.statusBar(), SLOT(showMessage(QString)));
    QObject::connect(&fakeVim, SIGNAL(quitRequested(QWidget *)),
        &app, SLOT(quit()));

    fakeVim.addWidget(widget);
    if (args.size() >= 1)
        fakeVim.handleCommand(widget, "r " + args.at(0));

    return app.exec();
}
