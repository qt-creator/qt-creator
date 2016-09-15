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

#pragma once

#include <QFrame>
#include <QPen>

QT_FORWARD_DECLARE_CLASS(QMouseEvent)
QT_FORWARD_DECLARE_CLASS(QPaintEvent)
QT_FORWARD_DECLARE_CLASS(QResizeEvent)

namespace ScxmlEditor {

namespace Common {

class ColorThemeItem : public QFrame
{
    Q_OBJECT

public:
    ColorThemeItem(int index, const QColor &color, QWidget *parent = nullptr);

    void setIndex(int index);
    QColor color() const;
    void setColor(const QColor &color);

signals:
    void mousePressed();
    void colorChanged();

protected:
    void paintEvent(QPaintEvent *e) override;
    void enterEvent(QEvent *e) override;
    void leaveEvent(QEvent *e) override;
    void mousePressEvent(QMouseEvent *e) override;

private:
    void openColorDialog();

    int m_index;
    QColor m_color;
    QPen m_pen;
};

class ColorThemeView : public QFrame
{
    Q_OBJECT

public:
    explicit ColorThemeView(QWidget *parent = nullptr);

    void reset();
    void setColor(int index, const QColor &color);
    QColor color(int index) const;
    QVariantMap colorData() const;
    static const QVector<QColor> &defaultColors();

signals:
    void colorChanged();

protected:
    void resizeEvent(QResizeEvent *e) override;

private:
    void updateItemRects();
    ColorThemeItem *createItem(int index, const QColor &color);

    QVector<ColorThemeItem*> m_themeItems;
};

} // namespace Common
} // namespace ScxmlEditor
