
#include "fakevimhandler.h"

#include <QtCore/QDebug>

#include <QtGui/QApplication>
#include <QtGui/QMainWindow>
#include <QtGui/QMessageBox>
#include <QtGui/QPlainTextEdit>
#include <QtGui/QStatusBar>
#include <QtGui/QTextEdit>

using namespace FakeVim::Internal;

class Proxy : public QObject
{
    Q_OBJECT

public:
    Proxy(QWidget *widget, QObject *parent = 0)
      : QObject(parent), m_widget(widget)
    {}

public slots:
    void changeSelection(const QList<QTextEdit::ExtraSelection> &s)
    {
        if (QPlainTextEdit *ed = qobject_cast<QPlainTextEdit *>(m_widget))
            ed->setExtraSelections(s);
        else if (QTextEdit *ed = qobject_cast<QTextEdit *>(m_widget))
            ed->setExtraSelections(s);
    }

    void changeExtraInformation(const QString &info)
    {
        QMessageBox::information(m_widget, "Information", info);
    }

private:
    QWidget *m_widget;
};

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
    widget->setObjectName("Editor");
    widget->resize(450, 350);
    widget->setFocus();

    Proxy proxy(widget);

    FakeVimHandler handler(widget, 0);

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

    QObject::connect(&handler, SIGNAL(commandBufferChanged(QString)),
        mw.statusBar(), SLOT(showMessage(QString)));
    QObject::connect(&handler, SIGNAL(quitRequested()),
        &app, SLOT(quit()));
    QObject::connect(&handler,
        SIGNAL(selectionChanged(QList<QTextEdit::ExtraSelection>)),
        &proxy, SLOT(changeSelection(QList<QTextEdit::ExtraSelection>)));
    QObject::connect(&handler,
        SIGNAL(extraInformationChanged(QString)),
        &proxy, SLOT(changeExtraInformation(QString)));

    handler.setupWidget();
    if (args.size() >= 1)
        handler.handleCommand("r " + args.at(0));

    return app.exec();
}

#include "main.moc"
