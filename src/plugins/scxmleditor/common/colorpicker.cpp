// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "colorpicker.h"
#include "scxmleditorconstants.h"
#include "scxmleditortr.h"

#include <utils/layoutbuilder.h>

#include <coreplugin/icore.h>

#include <QHBoxLayout>
#include <QToolButton>

using namespace Utils;

namespace ScxmlEditor::Common {

const char C_SETTINGS_COLORPICKER_LASTUSEDCOLORS[] = "ScxmlEditor/ColorPickerLastUsedColors_%1";
constexpr int C_BUTTON_COLUMNS_COUNT = 5;

ColorPicker::ColorPicker(const QString &key, QWidget *parent)
    : QFrame(parent)
    , m_key(key)
{
    const QVector<QRgb> colors = {
        qRgb(0xed, 0xf7, 0xf2), qRgb(0xdf, 0xd3, 0xb6), qRgb(0x89, 0x72, 0x5b), qRgb(0xff, 0xd3, 0x93), qRgb(0xff, 0x97, 0x4f),
        qRgb(0xff, 0x85, 0x0d), qRgb(0xf7, 0xe9, 0x67), qRgb(0xef, 0xc9, 0x4c), qRgb(0xff, 0xe1, 0x1a), qRgb(0xc2, 0xe0, 0x78),
        qRgb(0xa2, 0xd7, 0x00), qRgb(0x45, 0xbf, 0x08), qRgb(0x91, 0xe3, 0xd8), qRgb(0x3c, 0xb3, 0xfd), qRgb(0x34, 0x67, 0xba),
        qRgb(0xc5, 0xba, 0xfc), qRgb(0xb6, 0x65, 0xfc), qRgb(0xa5, 0x08, 0xd0), qRgb(0xcc, 0x56, 0x64), qRgb(0x96, 0x2d, 0x3e)
    };

    auto basicColorContentFrame = new QWidget;
    auto lastUsedColorContainer = new QWidget;

    m_lastUsedColorContainer = new QHBoxLayout(lastUsedColorContainer);
    m_lastUsedColorContainer->setContentsMargins(0, 0, 0, 0);

    using namespace Layouting;
    Grid colorGrid{noMargin};
    for (int i = 0; i < colors.count(); ++i) {
        QWidget *button = createButton(colors[i]);
        colorGrid.addItem(button);
        if ((i + 1) % C_BUTTON_COLUMNS_COUNT == 0)
            colorGrid.addItem(br);
        if (i == 0)
            m_lastUsedColorContainer->addSpacerItem(new QSpacerItem(0, button->sizeHint().height(),
                                                                    QSizePolicy::MinimumExpanding,
                                                                    QSizePolicy::Preferred));
    }
    colorGrid.attachTo(basicColorContentFrame);
    Column {
        Tr::tr("Basic Colors"),
        basicColorContentFrame,
        Tr::tr("Last used colors"),
        lastUsedColorContainer,
    }.attachTo(this);

    const QStringList lastColors = Core::ICore::settings()->value(
        C_SETTINGS_COLORPICKER_LASTUSEDCOLORS + keyFromString(m_key), QStringList()).toStringList();
    for (int i = lastColors.count(); i--;)
        setLastUsedColor(lastColors[i]);
}

ColorPicker::~ColorPicker()
{
    Core::ICore::settings()->setValue(
        C_SETTINGS_COLORPICKER_LASTUSEDCOLORS + keyFromString(m_key), m_lastUsedColorNames);
}

void ColorPicker::setLastUsedColor(const QString &colorName)
{
    if (colorName.isEmpty())
        return;

    if (m_lastUsedColorNames.contains(colorName))
        return;

    m_lastUsedColorNames.insert(0, colorName);
    m_lastUsedColorButtons.insert(0, createButton(colorName));

    while (m_lastUsedColorButtons.count() > C_BUTTON_COLUMNS_COUNT) {
        m_lastUsedColorButtons.takeLast()->deleteLater();
        m_lastUsedColorNames.takeLast();
    }

    m_lastUsedColorContainer->insertWidget(0, m_lastUsedColorButtons.first());
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

} // ScxmlEditor::Common
