#include "htmleditorwidget.h"

#include <QTextEdit>
#include <QPlainTextEdit>
#include <QtWebKit>
#include <QDebug>

struct HTMLEditorWidgetData
{
    QWebView* webView;
    QPlainTextEdit* textEdit;
    bool modified;
    QString path;
};

HTMLEditorWidget::HTMLEditorWidget(QWidget* parent):QTabWidget(parent)
{
    d = new HTMLEditorWidgetData;

    d->webView = new QWebView;
    d->textEdit = new QPlainTextEdit;

    addTab(d->webView, "Preview");
    addTab(d->textEdit, "Source");
    setTabPosition(QTabWidget::South);
    setTabShape(QTabWidget::Triangular);

    d->textEdit->setFont( QFont("Courier", 12) );

    connect(this, SIGNAL(currentChanged(int)),
            this, SLOT(slotCurrentTabChanged(int)));

    connect(d->textEdit, SIGNAL(textChanged()),
            this, SLOT(slotContentModified()));

    connect(d->webView, SIGNAL(titleChanged(QString)),
            this, SIGNAL(titleChanged(QString)));

    d->modified = false;
}


HTMLEditorWidget::~HTMLEditorWidget()
{
    delete d;
}

void HTMLEditorWidget::setContent(const QByteArray& ba, const QString& path)
{
    if(path.isEmpty())
        d->webView->setHtml(ba);
    else
        d->webView->setHtml(ba, "file:///" + path);
    d->textEdit->setPlainText(ba);
    d->modified = false;
    d->path = path;
}

QByteArray HTMLEditorWidget::content() const
{
    QString HTMLText = d->textEdit->toPlainText();
    return HTMLText.toAscii();
}

QString HTMLEditorWidget::title() const
{
    return d->webView->title();
}

void HTMLEditorWidget::slotCurrentTabChanged(int tab)
{
    if(tab == 0 && d->modified)
        setContent( content(), d->path );
}

void HTMLEditorWidget::slotContentModified()
{
    d->modified = true;
    emit contentModified();
}


