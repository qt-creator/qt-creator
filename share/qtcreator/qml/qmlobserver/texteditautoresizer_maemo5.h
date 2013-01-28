/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include <qplaintextedit.h>
#include <qtextedit.h>
#include <qabstractkineticscroller.h>
#include <qscrollarea.h>
#include <QDebug>

#ifndef TEXTEDITAUTORESIZER_H
#define TEXTEDITAUTORESIZER_H

class TextEditAutoResizer : public QObject
{
    Q_OBJECT
public:
    TextEditAutoResizer(QWidget *parent)
        : QObject(parent), plainTextEdit(qobject_cast<QPlainTextEdit *>(parent)),
          textEdit(qobject_cast<QTextEdit *>(parent)), edit(qobject_cast<QFrame *>(parent))
    {
        // parent must either inherit QPlainTextEdit or  QTextEdit!
        Q_ASSERT(plainTextEdit || textEdit);

        connect(parent, SIGNAL(textChanged()), this, SLOT(textEditChanged()));
        connect(parent, SIGNAL(cursorPositionChanged()), this, SLOT(textEditChanged()));

        textEditChanged();
    }

private Q_SLOTS:
    inline void textEditChanged();

private:
    QPlainTextEdit *plainTextEdit;
    QTextEdit *textEdit;
    QFrame *edit;
};

void TextEditAutoResizer::textEditChanged()
{
    QTextDocument *doc = textEdit ? textEdit->document() : plainTextEdit->document();
    QRect cursor = textEdit ? textEdit->cursorRect() : plainTextEdit->cursorRect();

    QSize s = doc->size().toSize();
    if (plainTextEdit)
        s.setHeight((s.height() + 2) * edit->fontMetrics().lineSpacing());

    const QRect fr = edit->frameRect();
    const QRect cr = edit->contentsRect();

    edit->setMinimumHeight(qMax(70, s.height() + (fr.height() - cr.height() - 1)));

    // make sure the cursor is visible in case we have a QAbstractScrollArea parent
    QPoint pos = edit->pos();
    QWidget *pw = edit->parentWidget();
    while (pw) {
        if (qobject_cast<QScrollArea *>(pw))
            break;
        pw = pw->parentWidget();
    }

    if (pw) {
        QScrollArea *area = static_cast<QScrollArea *>(pw);
        QPoint scrollto = area->widget()->mapFrom(edit, cursor.center());
        QPoint margin(10 + cursor.width(), 2 * cursor.height());

        if (QAbstractKineticScroller *scroller = area->property("kineticScroller").value<QAbstractKineticScroller *>()) {
            scroller->ensureVisible(scrollto, margin.x(), margin.y());
        } else {
            area->ensureVisible(scrollto.x(), scrollto.y(), margin.x(), margin.y());
        }
    }
}

#endif
