/***************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (info@qt.nokia.com)
**
** This file is part of the documentation of Qt Creator.
**
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Nokia Corporation and its Subsidiary(-ies) nor
**     the names of its contributors may be used to endorse or promote
**     products derived from this software without specific prior written
**     permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
****************************************************************************/

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


