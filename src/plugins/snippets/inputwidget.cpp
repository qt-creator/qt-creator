/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.2, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/
***************************************************************************/
#include <QtCore/QDebug>
#include <QtGui/QApplication>
#include <QtGui/QLabel>
#include <QtGui/QLineEdit>
#include <QtGui/QHBoxLayout>
#include <QtGui/QBitmap>
#include <QtGui/QPainter>
#include <QtGui/QResizeEvent>

#include "inputwidget.h"

using namespace Snippets::Internal;

InputWidget::InputWidget(const QString &text, const QString &value)
    : QFrame(0, Qt::Popup)
{
    setAttribute(Qt::WA_DeleteOnClose);

    m_label = new QLabel();
    m_label->setTextFormat(Qt::RichText);
    m_label->setText(text);

    m_lineEdit = new QLineEdit();
    m_lineEdit->setText(value);
    m_lineEdit->setSelection(0, value.length());

    qApp->installEventFilter(this);

    QHBoxLayout *layout = new QHBoxLayout;
    layout->addWidget(m_label);
    layout->addWidget(m_lineEdit);
    layout->setMargin(3);

    setLayout(layout);
    ensurePolished();

    setAutoFillBackground(false);
}

void InputWidget::resizeEvent(QResizeEvent *event)
{
    int height = event->size().height();
    int width = event->size().width();
    qDebug() << event->size();

    QPalette pal = palette();
    QLinearGradient bg(0,0,0,height);
    bg.setColorAt(0, QColor(195,195,255));
    bg.setColorAt(1, QColor(230,230,255));
    pal.setBrush(QPalette::Background, QBrush(bg));
    setPalette(pal);

    QBitmap bm(width, height);
    bm.fill(Qt::color0);
    QPainter p(&bm);
    p.setBrush(QBrush(Qt::color1, Qt::SolidPattern));
    p.setPen(Qt::color1);
    int rw = (25 * height) / width;
    p.drawRoundRect(0,0,width,height, rw, 25);
    setMask(bm);
}

void InputWidget::showInputWidget(const QPoint &position)
{
    move(position);
    show();
    m_lineEdit->setFocus();
}

bool InputWidget::eventFilter(QObject *o, QEvent *e)
{
    if (o != m_lineEdit) {
        switch (e->type()) {
        case QEvent::MouseButtonPress:
        case QEvent::MouseButtonDblClick:
            closeInputWidget(true);
        default:
            break;
        }
    } else if (e->type() == QEvent::KeyPress) {
        QKeyEvent *ke = static_cast<QKeyEvent *>(e);
        switch (ke->key()) {
            case Qt::Key_Escape:
                qDebug() << "Escape";
                closeInputWidget(true);
                break;
            case Qt::Key_Enter:
            case Qt::Key_Return:
                qDebug() << "Enter";
                closeInputWidget(false);
                return true;
        }
    }

    return false;
}

void InputWidget::closeInputWidget(bool cancel)
{
    emit finished(cancel, m_lineEdit->text());
    close();
}
