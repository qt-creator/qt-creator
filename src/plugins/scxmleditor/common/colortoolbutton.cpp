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

#include "colortoolbutton.h"
#include "colorpicker.h"
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

    connect(this, &ColorToolButton::clicked, this, [this]() {
        setCurrentColor(m_color);
    });

    QPixmap p(15, 15);
    p.fill(Qt::black);

    m_colorPickerAction = new ColorPickerAction(key, this);
    connect(m_colorPickerAction, &ColorPickerAction::colorSelected, this, &ColorToolButton::setCurrentColor);
    connect(this, &ColorToolButton::colorSelected, m_colorPickerAction, &ColorPickerAction::lastUsedColor);

    m_menu = new QMenu(this);
    m_menu->addAction(QIcon(p), tr("Automatic Color"), this, &ColorToolButton::autoColorSelected);
    m_menu->addSeparator();
    m_menu->addAction(m_colorPickerAction);
    m_menu->addSeparator();
    m_menu->addAction(QIcon(QPixmap(":/scxmleditor/images/more_colors.png")), tr("More Colors..."), this, &ColorToolButton::showColorDialog);
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
