// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
    void enterEvent(QEnterEvent *e) override;
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
