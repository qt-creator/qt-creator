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

#include "colorpicker.h"
#include "scxmleditorconstants.h"

#include <QToolButton>

#include <coreplugin/icore.h>

using namespace ScxmlEditor::Common;

const char C_SETTINGS_COLORPICKER_LASTUSEDCOLORS[] = "ScxmlEditor/ColorPickerLastUsedColors_%1";

ColorPicker::ColorPicker(const QString &key, QWidget *parent)
    : QFrame(parent)
    , m_key(key)
{
    m_ui.setupUi(this);

    const QVector<QRgb> colors = {
        qRgb(0xed, 0xf7, 0xf2), qRgb(0xdf, 0xd3, 0xb6), qRgb(0x89, 0x72, 0x5b), qRgb(0xff, 0xd3, 0x93), qRgb(0xff, 0x97, 0x4f),
        qRgb(0xff, 0x85, 0x0d), qRgb(0xf7, 0xe9, 0x67), qRgb(0xef, 0xc9, 0x4c), qRgb(0xff, 0xe1, 0x1a), qRgb(0xc2, 0xe0, 0x78),
        qRgb(0xa2, 0xd7, 0x00), qRgb(0x45, 0xbf, 0x08), qRgb(0x91, 0xe3, 0xd8), qRgb(0x3c, 0xb3, 0xfd), qRgb(0x34, 0x67, 0xba),
        qRgb(0xc5, 0xba, 0xfc), qRgb(0xb6, 0x65, 0xfc), qRgb(0xa5, 0x08, 0xd0), qRgb(0xcc, 0x56, 0x64), qRgb(0x96, 0x2d, 0x3e)
    };

    auto vBoxLayout = new QVBoxLayout;
    vBoxLayout->setContentsMargins(0, 0, 0, 0);
    vBoxLayout->setMargin(0);
    vBoxLayout->setSpacing(0);

    const int buttonRowsCount = 4;
    const int buttonColumnsCount = 5;

    for (int r = 0; r < buttonRowsCount; ++r) {
        auto hBoxLayout = new QHBoxLayout;
        hBoxLayout->setContentsMargins(0, 0, 0, 0);
        hBoxLayout->setMargin(0);
        hBoxLayout->setSpacing(0);

        for (int c = 0; c < buttonColumnsCount; ++c)
            hBoxLayout->addWidget(createButton(colors[r * buttonColumnsCount + c]));

        hBoxLayout->addStretch();
        vBoxLayout->addLayout(hBoxLayout);
    }

    m_ui.basicColorContentFrame->setLayout(vBoxLayout);

    const QStringList lastColors = Core::ICore::settings()->value(
                QString::fromLatin1(C_SETTINGS_COLORPICKER_LASTUSEDCOLORS).arg(m_key), QStringList()).toStringList();
    for (int i = lastColors.count(); i--;)
        setLastUsedColor(lastColors[i]);
}

ColorPicker::~ColorPicker()
{
    Core::ICore::settings()->setValue(QString::fromLatin1(C_SETTINGS_COLORPICKER_LASTUSEDCOLORS).arg(m_key),
                                      m_lastUsedColorNames);
}

void ColorPicker::setLastUsedColor(const QString &colorName)
{
    if (colorName.isEmpty())
        return;

    if (m_lastUsedColorNames.contains(colorName))
        return;

    m_lastUsedColorNames.insert(0, colorName);
    m_lastUsedColorButtons.insert(0, createButton(colorName));

    while (m_lastUsedColorButtons.count() > 5) {
        m_lastUsedColorButtons.takeLast()->deleteLater();
        m_lastUsedColorNames.takeLast();
    }

    m_ui.lastUsedColorLayout->insertWidget(0, m_lastUsedColorButtons.first());
}

QToolButton *ColorPicker::createButton(const QColor &color)
{
    auto button = new QToolButton;
    button->setObjectName("colorPickerButton");
    QPixmap pixmap(15, 15);
    pixmap.fill(color);
    button->setIcon(QIcon(pixmap));

    connect(button, &QToolButton::clicked, this, [this, color] {
        emit colorSelected(QColor(color).name());
    });

    return button;
}
