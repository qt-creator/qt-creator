/***************************************************************************
**
** Copyright (C) 2015 Jochen Becher
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "palettebox.h"
#include "qmt/infrastructure/qmtassert.h"

#include <QPainter>
#include <QMouseEvent>
#include <QKeyEvent>

namespace qmt {

PaletteBox::PaletteBox(QWidget *parent)
    : QWidget(parent),
      _brushes(6),
      _pens(6),
      _current_index(-1)
{
    setFocusPolicy(Qt::StrongFocus);
}

PaletteBox::~PaletteBox()
{
}

QBrush PaletteBox::getBrush(int index) const
{
    QMT_CHECK(index >= 0 && index <= _brushes.size());
    return _brushes.at(index);
}

void PaletteBox::setBrush(int index, const QBrush &brush)
{
    QMT_CHECK(index >= 0 && index <= _brushes.size());
    if (_brushes[index] != brush) {
        _brushes[index] = brush;
        update();
    }
}

QPen PaletteBox::getLinePen(int index) const
{
    QMT_CHECK(index >= 0 && index <= _pens.size());
    return _pens.at(index);
}

void PaletteBox::setLinePen(int index, const QPen &pen)
{
    QMT_CHECK(index >= 0 && index <= _pens.size());
    if (_pens[index] != pen) {
        _pens[index] = pen;
        update();
    }
}

void PaletteBox::clear()
{
    setCurrentIndex(-1);
}

void PaletteBox::setCurrentIndex(int index)
{
    if (_current_index != index) {
        if (index >= 0 && index < _brushes.size()) {
            _current_index = index;
        } else {
            _current_index = -1;
        }
        update();
    }
}

void PaletteBox::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    qreal w = (qreal) width() / (qreal) _brushes.size();
    qreal h = height();
    for (int i = 0; i < _brushes.size(); ++i) {
        QBrush brush = _brushes.at(i);
        if (i == _current_index) {
            painter.fillRect(QRectF(i * w, 0, w, h), brush);
            QPen pen = _pens.at(i);
            pen.setWidth(2);
            painter.setPen(pen);
            painter.drawRect(QRectF(i * w + 1, 1, w - 2, h - 2));
        } else {
            painter.fillRect(QRectF(i * w, 0, w, h), brush);
        }
    }
    if (hasFocus()) {
        painter.setBrush(Qt::NoBrush);
        QPen pen;
        pen.setColor(Qt::black);
        pen.setStyle(Qt::DotLine);
        painter.setPen(pen);
        painter.drawRect(0, 0, width() - 1, height() - 1);
    }
}

void PaletteBox::mousePressEvent(QMouseEvent *event)
{
    qreal w = (qreal) width() / (qreal) _brushes.size();

    int i = (int) (event->x() / w);
    QMT_CHECK(i >= 0 && i < _brushes.size());
    setCurrentIndex(i);
    if (_current_index >= 0 && _current_index < _brushes.size()) {
        emit activated(_current_index);
    }
}

void PaletteBox::keyPressEvent(QKeyEvent *event)
{
    bool is_known_key = false;
    switch (event->key()) {
    case Qt::Key_Left:
        if (_current_index <= 0) {
            setCurrentIndex((_brushes.size() - 1));
        } else {
            setCurrentIndex(_current_index - 1);
        }
        is_known_key = true;
        break;
    case Qt::Key_Right:
        if (_current_index < 0 || _current_index >= _brushes.size() - 1) {
            setCurrentIndex(0);
        } else {
            setCurrentIndex(_current_index + 1);
        }
        is_known_key = true;
        break;
    }
    if (is_known_key && _current_index >= 0 && _current_index < _brushes.size()) {
        emit activated(_current_index);
    }
}

} // namespace qmt
