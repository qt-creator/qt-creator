// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "colorsettings.h"
#include "colorthemeview.h"
#include "scxmleditorconstants.h"
#include "scxmleditortr.h"

#include <QComboBox>
#include <QInputDialog>
#include <QMessageBox>
#include <QToolButton>

#include <coreplugin/icore.h>

#include <utils/layoutbuilder.h>
#include <utils/utilsicons.h>

using namespace Utils;

namespace ScxmlEditor::Common {

ColorSettings::ColorSettings(QWidget *parent)
    : QFrame(parent)
{
    m_colorThemeView = new ColorThemeView;
    m_colorThemeView->setEnabled(false);
    m_comboColorThemes = new QComboBox;
    auto addTheme = new QToolButton;
    addTheme->setIcon(Utils::Icons::PLUS.icon());
    addTheme->setAutoRaise(true);
    auto removeTheme = new QToolButton;
    removeTheme->setIcon(Utils::Icons::MINUS.icon());
    removeTheme->setAutoRaise(true);
    const QtcSettings *s = Core::ICore::settings();
    m_colorThemes = s->value(Constants::C_SETTINGS_COLORSETTINGS_COLORTHEMES).toMap();
    m_comboColorThemes->addItems(m_colorThemes.keys());
    m_comboColorThemes->setCurrentText(
                s->value(Constants::C_SETTINGS_COLORSETTINGS_CURRENTCOLORTHEME).toString());
    selectTheme(m_comboColorThemes->currentIndex());

    using namespace Layouting;
    Column {
        Row {
            m_comboColorThemes,
            addTheme,
            removeTheme,
        },
        m_colorThemeView,
        noMargin
    }.attachTo(this);

    connect(m_comboColorThemes, &QComboBox::currentIndexChanged,
            this, &ColorSettings::selectTheme);
    connect(m_colorThemeView, &ColorThemeView::colorChanged,
            this, &ColorSettings::updateCurrentColors);
    connect(addTheme, &QToolButton::clicked, this, &ColorSettings::createTheme);
    connect(removeTheme, &QToolButton::clicked, this, &ColorSettings::removeTheme);
}

void ColorSettings::save()
{
    QtcSettings *s = Core::ICore::settings();
    s->setValue(Constants::C_SETTINGS_COLORSETTINGS_COLORTHEMES, m_colorThemes);
    s->setValue(Constants::C_SETTINGS_COLORSETTINGS_CURRENTCOLORTHEME, m_comboColorThemes->currentText());
}

void ColorSettings::updateCurrentColors()
{
    m_colorThemes[m_comboColorThemes->currentText()] = m_colorThemeView->colorData();
}

void ColorSettings::selectTheme(int index)
{
    const QString name = m_comboColorThemes->itemText(index);
    m_colorThemeView->reset();
    if (!name.isEmpty() && m_colorThemes.contains(name)) {
        m_colorThemeView->setEnabled(true);
        const QVariantMap colordata = m_colorThemes[name].toMap();
        for (auto it = colordata.cbegin(); it != colordata.cend(); ++it)
            m_colorThemeView->setColor(it.key().toInt(), QColor(it.value().toString()));
    } else {
        m_colorThemeView->setEnabled(false);
    }
}

void ColorSettings::createTheme()
{
    QString name = QInputDialog::getText(this, Tr::tr("Create New Color Theme"), Tr::tr("Theme ID"));
    if (!name.isEmpty()) {
        if (m_colorThemes.contains(name)) {
            QMessageBox::warning(this, Tr::tr("Cannot Create Theme"), Tr::tr("Theme %1 is already available.").arg(name));
        } else {
            m_colorThemeView->reset();
            m_colorThemes[name] = QVariantMap();
            m_comboColorThemes->addItem(name);
            m_comboColorThemes->setCurrentText(name);
        }
    }
}

void ColorSettings::removeTheme()
{
    const QString name = m_comboColorThemes->currentText();
    const QMessageBox::StandardButton result = QMessageBox::question(this, Tr::tr("Remove Color Theme"),
        Tr::tr("Are you sure you want to delete color theme %1?").arg(name),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (result == QMessageBox::Yes) {
        m_comboColorThemes->removeItem(m_comboColorThemes->currentIndex());
        m_colorThemes.remove(name);
        m_comboColorThemes->setCurrentIndex(0);
        if (m_colorThemes.isEmpty())
            m_colorThemeView->setEnabled(false);
    }
}

} // ScxmlEditor::Common
