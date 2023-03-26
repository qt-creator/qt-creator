// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "colortoolbutton.h"
#include "colorpicker.h"
#include "scxmleditortr.h"

#include <QColorDialog>
#include <QPainter>

using namespace ScxmlEditor::Common;

ColorPickerAction::ColorPickerAction(const QString &key, QObject *parent)
    : QWidgetAction(parent)
    , m_key(key)
{
}

QWidget *ColorPickerAction::createWidget(QWidget *parent)
{
    auto picker = new ColorPicker(m_key, parent);
    connect(picker, &ColorPicker::colorSelected, this, &ColorPickerAction::colorSelected);
    connect(this, &ColorPickerAction::lastUsedColor, picker, &ColorPicker::setLastUsedColor);

    return picker;
}

ColorToolButton::ColorToolButton(const QString &key, const QString &iconName, const QString &tooltip, QWidget *parent)
    : QToolButton(parent)
{
    setIcon(QIcon(iconName));
    setToolTip(tooltip);
    setPopupMode(QToolButton::MenuButtonPopup);

    connect(this, &ColorToolButton::clicked, this, [this] {
        setCurrentColor(m_color);
    });

    QPixmap p(15, 15);
    p.fill(Qt::black);

    m_colorPickerAction = new ColorPickerAction(key, this);
    connect(m_colorPickerAction, &ColorPickerAction::colorSelected, this, &ColorToolButton::setCurrentColor);
    connect(this, &ColorToolButton::colorSelected, m_colorPickerAction, &ColorPickerAction::lastUsedColor);

    m_menu = new QMenu(this);
    m_menu->addAction(QIcon(p), Tr::tr("Automatic Color"), this, &ColorToolButton::autoColorSelected);
    m_menu->addSeparator();
    m_menu->addAction(m_colorPickerAction);
    m_menu->addSeparator();
    m_menu->addAction(QIcon(QPixmap(":/scxmleditor/images/more_colors.png")), Tr::tr("More Colors..."), this, &ColorToolButton::showColorDialog);
    setMenu(m_menu);
}

ColorToolButton::~ColorToolButton()
{
    m_menu->deleteLater();
}

void ColorToolButton::autoColorSelected()
{
    setCurrentColor(QString());
}

void ColorToolButton::showColorDialog()
{
    QColor c = QColorDialog::getColor();
    if (c.isValid())
        setCurrentColor(c.name());
}

void ColorToolButton::setCurrentColor(const QString &currentColor)
{
    menu()->hide();
    m_color = currentColor;
    emit colorSelected(m_color);
    update();
}

void ColorToolButton::paintEvent(QPaintEvent *e)
{
    QToolButton::paintEvent(e);

    QPainter p(this);
    QRect r(2, height() - 7, width() - 17, 4);
    p.fillRect(r, QBrush(QColor(m_color)));

    if (!isEnabled()) {
        QColor c(Qt::gray);
        c.setAlpha(200);
        p.fillRect(r, QBrush(c));
    }
}
