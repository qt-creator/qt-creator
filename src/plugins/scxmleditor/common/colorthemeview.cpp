/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "colorthemeview.h"

#include <QBrush>
#include <QColorDialog>
#include <QLinearGradient>
#include <QPainter>
#include <QVariantMap>

using namespace ScxmlEditor::Common;

ColorThemeItem::ColorThemeItem(int index, const QColor &color, QWidget *parent)
    : QFrame(parent)
    , m_index(index)
    , m_color(color)
{
    m_pen = QPen(Qt::black);
    m_pen.setCosmetic(true);
    connect(this, &ColorThemeItem::mousePressed, this, &ColorThemeItem::openColorDialog);
}

void ColorThemeItem::setColor(const QColor &color)
{
    m_color = color;
    update();
}

QColor ColorThemeItem::color() const
{
    return m_color;
}

void ColorThemeItem::openColorDialog()
{
    QColor oldColor = m_color;

    QColorDialog dialog(oldColor, 0);
    dialog.setWindowTitle(tr("Pick Color"));
    connect(&dialog, &QColorDialog::currentColorChanged, this, &ColorThemeItem::setColor);
    QPoint topRight = parentWidget()->mapToGlobal(parentWidget()->rect().topRight());
    dialog.move(topRight.x(), topRight.y());
    if (dialog.exec() == QDialog::Accepted) {
        setColor(dialog.currentColor());
        emit colorChanged();
    } else {
        setColor(oldColor);
    }
}

void ColorThemeItem::enterEvent(QEvent *e)
{
    m_pen.setWidth(isEnabled() ? 3 : 1);

    update();
    QFrame::enterEvent(e);
}

void ColorThemeItem::leaveEvent(QEvent *e)
{
    m_pen.setWidth(1);
    update();
    QFrame::leaveEvent(e);
}

void ColorThemeItem::mousePressEvent(QMouseEvent *e)
{
    QFrame::mousePressEvent(e);
    emit mousePressed();
}

void ColorThemeItem::paintEvent(QPaintEvent *e)
{
    QFrame::paintEvent(e);

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    QRectF r = QRectF(rect()).adjusted(1.5, 1.5, -1.5, -1.5);
    QLinearGradient grad(r.topLeft(), r.bottomLeft());
    grad.setColorAt(0, m_color.lighter(115));
    grad.setColorAt(1, m_color);
    p.setBrush(QBrush(grad));
    p.setPen(m_pen);

    qreal radius = r.width() * 0.1;
    p.drawRoundedRect(r, radius, radius);
}

ColorThemeView::ColorThemeView(QWidget *parent)
    : QFrame(parent)
{
    for (int i = 0; i < defaultColors().count(); ++i) {
        m_themeItems << createItem(i, defaultColors()[i]);
        connect(m_themeItems[i], &ColorThemeItem::colorChanged, this, &ColorThemeView::colorChanged);
    }
}

void ColorThemeView::reset()
{
    for (int i = 0; i < m_themeItems.count(); ++i)
        m_themeItems[i]->setColor(defaultColors()[i]);
}

void ColorThemeView::setColor(int index, const QColor &color)
{
    if (index >= 0 && index < m_themeItems.count())
        m_themeItems[index]->setColor(color);
}

QColor ColorThemeView::color(int index) const
{
    if (index >= 0 && index < m_themeItems.count())
        return m_themeItems[index]->color();

    return QColor();
}

QVariantMap ColorThemeView::colorData() const
{
    QVariantMap data;
    for (int i = 0; i < m_themeItems.count(); ++i) {
        if (m_themeItems[i]->color() != defaultColors()[i])
            data[QString::fromLatin1("%1").arg(i)] = m_themeItems[i]->color().name();
    }

    return data;
}

ColorThemeItem *ColorThemeView::createItem(int index, const QColor &color)
{
    return new ColorThemeItem(index, color, this);
}

const QVector<QColor> &ColorThemeView::defaultColors()
{
    static const QVector<QColor> colors = {
        QColor(0xe0, 0xe0, 0xe0),
        QColor(0xd3, 0xe4, 0xc3),
        QColor(0xeb, 0xe4, 0xba),
        QColor(0xb8, 0xdd, 0xeb),
        QColor(0xc7, 0xc8, 0xdd),
        QColor(0xf0, 0xce, 0xa5),
        QColor(0xf1, 0xba, 0xba)
    };
    return colors;
}

void ColorThemeView::resizeEvent(QResizeEvent *e)
{
    QFrame::resizeEvent(e);
    updateItemRects();
}

void ColorThemeView::updateItemRects()
{
    int size = qMin(width() / 2, height() / 2);

    int capx = size / defaultColors().count();
    int capy = capx;

    for (int i = 0; i < m_themeItems.count(); ++i) {
        m_themeItems[i]->resize(size, size);
        m_themeItems[i]->move(QPoint(capx * (i + 1), capy * (i + 1)));
    }
}
