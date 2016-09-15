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

#include "colorsettings.h"
#include "scxmleditorconstants.h"

#include <QInputDialog>
#include <QMessageBox>

#include <coreplugin/icore.h>

using namespace ScxmlEditor::Common;

ColorSettings::ColorSettings(QWidget *parent)
    : QFrame(parent)
{
    m_ui.setupUi(this);

    m_ui.m_colorThemeView->setEnabled(false);
    connect(m_ui.m_comboColorThemes, static_cast<void (QComboBox::*)(const QString&)>(&QComboBox::currentIndexChanged), this, &ColorSettings::selectTheme);
    connect(m_ui.m_colorThemeView, &ColorThemeView::colorChanged, this, &ColorSettings::updateCurrentColors);
    connect(m_ui.m_addColorTheme, &QToolButton::clicked, this, &ColorSettings::createTheme);
    connect(m_ui.m_removeColorTheme, &QToolButton::clicked, this, &ColorSettings::removeTheme);

    const QSettings *s = Core::ICore::settings();

    m_colorThemes = s->value(Constants::C_SETTINGS_COLORSETTINGS_COLORTHEMES).toMap();

    m_ui.m_comboColorThemes->clear();
    foreach (const QString &key, m_colorThemes.keys())
        m_ui.m_comboColorThemes->addItem(key);

    m_ui.m_comboColorThemes->setCurrentText(s->value(Constants::C_SETTINGS_COLORSETTINGS_CURRENTCOLORTHEME).toString());
}

void ColorSettings::save()
{
    QSettings *s = Core::ICore::settings();
    s->setValue(Constants::C_SETTINGS_COLORSETTINGS_COLORTHEMES, m_colorThemes);
    s->setValue(Constants::C_SETTINGS_COLORSETTINGS_CURRENTCOLORTHEME, m_ui.m_comboColorThemes->currentText());
}

void ColorSettings::updateCurrentColors()
{
    m_colorThemes[m_ui.m_comboColorThemes->currentText()] = m_ui.m_colorThemeView->colorData();
}

void ColorSettings::selectTheme(const QString &name)
{
    m_ui.m_colorThemeView->reset();
    if (!name.isEmpty() && m_colorThemes.contains(name)) {
        m_ui.m_colorThemeView->setEnabled(true);
        QVariantMap colordata = m_colorThemes[name].toMap();
        foreach (const QString &index, colordata.keys())
            m_ui.m_colorThemeView->setColor(index.toInt(), QColor(colordata[index].toString()));
    } else {
        m_ui.m_colorThemeView->setEnabled(false);
    }
}

void ColorSettings::createTheme()
{
    QString name = QInputDialog::getText(this, tr("Create New Color Theme"), tr("Theme ID"));
    if (!name.isEmpty()) {
        if (m_colorThemes.keys().contains(name)) {
            QMessageBox::warning(this, tr("Cannot Create Theme"), tr("Theme %1 is already available.").arg(name));
        } else {
            m_ui.m_colorThemeView->reset();
            m_colorThemes[name] = QVariantMap();
            m_ui.m_comboColorThemes->addItem(name);
            m_ui.m_comboColorThemes->setCurrentText(name);
        }
    }
}

void ColorSettings::removeTheme()
{
    const QString name = m_ui.m_comboColorThemes->currentText();
    const QMessageBox::StandardButton result = QMessageBox::question(this, tr("Remove Color Theme"),
        tr("Are you sure you want to delete color theme %1?").arg(name),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (result == QMessageBox::Yes) {
        m_ui.m_comboColorThemes->removeItem(m_ui.m_comboColorThemes->currentIndex());
        m_colorThemes.remove(name);
        m_ui.m_comboColorThemes->setCurrentIndex(0);
        if (m_colorThemes.isEmpty())
            m_ui.m_colorThemeView->setEnabled(false);
    }
}
