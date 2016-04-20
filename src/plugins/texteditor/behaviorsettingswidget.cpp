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

#include "behaviorsettingswidget.h"
#include "ui_behaviorsettingswidget.h"

#include "tabsettingswidget.h"

#include <texteditor/typingsettings.h>
#include <texteditor/storagesettings.h>
#include <texteditor/behaviorsettings.h>
#include <texteditor/extraencodingsettings.h>
#include <utils/algorithm.h>

#include <QList>
#include <QString>
#include <QByteArray>
#include <QTextCodec>
#include <QTextStream>

#include <functional>

namespace TextEditor {

struct BehaviorSettingsWidgetPrivate
{
    Internal::Ui::BehaviorSettingsWidget m_ui;
    QList<QTextCodec *> m_codecs;
};

BehaviorSettingsWidget::BehaviorSettingsWidget(QWidget *parent)
    : QWidget(parent)
    , d(new BehaviorSettingsWidgetPrivate)
{
    d->m_ui.setupUi(this);

    QList<int> mibs = QTextCodec::availableMibs();
    Utils::sort(mibs);
    QList<int>::iterator firstNonNegative =
        std::find_if(mibs.begin(), mibs.end(), std::bind2nd(std::greater_equal<int>(), 0));
    if (firstNonNegative != mibs.end())
        std::rotate(mibs.begin(), firstNonNegative, mibs.end());
    foreach (int mib, mibs) {
        if (QTextCodec *codec = QTextCodec::codecForMib(mib)) {
            QString compoundName = QLatin1String(codec->name());
            foreach (const QByteArray &alias, codec->aliases()) {
                compoundName += QLatin1String(" / ");
                compoundName += QString::fromLatin1(alias);
            }
            d->m_ui.encodingBox->addItem(compoundName);
            d->m_codecs.append(codec);
        }
    }

    // Qt5 doesn't list the system locale (QTBUG-34283), so add it manually
    const QString system(QLatin1String("System"));
    if (d->m_ui.encodingBox->findText(system) == -1) {
        d->m_ui.encodingBox->insertItem(0, system);
        d->m_codecs.prepend(QTextCodec::codecForLocale());
    }

    auto currentIndexChanged = static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged);
    connect(d->m_ui.autoIndent, &QAbstractButton::toggled,
            this, &BehaviorSettingsWidget::slotTypingSettingsChanged);
    connect(d->m_ui.smartBackspaceBehavior, currentIndexChanged,
            this, &BehaviorSettingsWidget::slotTypingSettingsChanged);
    connect(d->m_ui.tabKeyBehavior, currentIndexChanged,
            this, &BehaviorSettingsWidget::slotTypingSettingsChanged);
    connect(d->m_ui.cleanWhitespace, &QAbstractButton::clicked,
            this, &BehaviorSettingsWidget::slotStorageSettingsChanged);
    connect(d->m_ui.inEntireDocument, &QAbstractButton::clicked,
            this, &BehaviorSettingsWidget::slotStorageSettingsChanged);
    connect(d->m_ui.addFinalNewLine, &QAbstractButton::clicked,
            this, &BehaviorSettingsWidget::slotStorageSettingsChanged);
    connect(d->m_ui.cleanIndentation, &QAbstractButton::clicked,
            this, &BehaviorSettingsWidget::slotStorageSettingsChanged);
    connect(d->m_ui.mouseHiding, &QAbstractButton::clicked,
            this, &BehaviorSettingsWidget::slotBehaviorSettingsChanged);
    connect(d->m_ui.mouseNavigation, &QAbstractButton::clicked,
            this, &BehaviorSettingsWidget::slotBehaviorSettingsChanged);
    connect(d->m_ui.scrollWheelZooming, &QAbstractButton::clicked,
            this, &BehaviorSettingsWidget::slotBehaviorSettingsChanged);
    connect(d->m_ui.camelCaseNavigation, &QAbstractButton::clicked,
            this, &BehaviorSettingsWidget::slotBehaviorSettingsChanged);
    connect(d->m_ui.utf8BomBox, currentIndexChanged,
            this, &BehaviorSettingsWidget::slotExtraEncodingChanged);
    connect(d->m_ui.encodingBox, currentIndexChanged,
            this, &BehaviorSettingsWidget::slotEncodingBoxChanged);
    connect(d->m_ui.constrainTooltipsBox, currentIndexChanged,
            this, &BehaviorSettingsWidget::slotBehaviorSettingsChanged);
    connect(d->m_ui.keyboardTooltips, &QAbstractButton::clicked,
            this, &BehaviorSettingsWidget::slotBehaviorSettingsChanged);
    connect(d->m_ui.smartSelectionChanging, &QAbstractButton::clicked,
            this, &BehaviorSettingsWidget::slotBehaviorSettingsChanged);
}

BehaviorSettingsWidget::~BehaviorSettingsWidget()
{
    delete d;
}

void BehaviorSettingsWidget::setActive(bool active)
{
    d->m_ui.tabPreferencesWidget->setEnabled(active);
    d->m_ui.groupBoxTyping->setEnabled(active);
    d->m_ui.groupBoxEncodings->setEnabled(active);
    d->m_ui.groupBoxMouse->setEnabled(active);
    d->m_ui.groupBoxStorageSettings->setEnabled(active);
}

void BehaviorSettingsWidget::setAssignedCodec(QTextCodec *codec)
{
    for (int i = 0; i < d->m_codecs.size(); ++i) {
        if (codec == d->m_codecs.at(i)) {
            d->m_ui.encodingBox->setCurrentIndex(i);
            break;
        }
    }
}

QTextCodec *BehaviorSettingsWidget::assignedCodec() const
{
    return d->m_codecs.at(d->m_ui.encodingBox->currentIndex());
}

void BehaviorSettingsWidget::setCodeStyle(ICodeStylePreferences *preferences)
{
    d->m_ui.tabPreferencesWidget->setPreferences(preferences);
}

void BehaviorSettingsWidget::setAssignedTypingSettings(const TypingSettings &typingSettings)
{
    d->m_ui.autoIndent->setChecked(typingSettings.m_autoIndent);
    d->m_ui.smartBackspaceBehavior->setCurrentIndex(typingSettings.m_smartBackspaceBehavior);
    d->m_ui.tabKeyBehavior->setCurrentIndex(typingSettings.m_tabKeyBehavior);
}

void BehaviorSettingsWidget::assignedTypingSettings(TypingSettings *typingSettings) const
{
    typingSettings->m_autoIndent = d->m_ui.autoIndent->isChecked();
    typingSettings->m_smartBackspaceBehavior =
        (TypingSettings::SmartBackspaceBehavior)(d->m_ui.smartBackspaceBehavior->currentIndex());
    typingSettings->m_tabKeyBehavior =
        (TypingSettings::TabKeyBehavior)(d->m_ui.tabKeyBehavior->currentIndex());
}

void BehaviorSettingsWidget::setAssignedStorageSettings(const StorageSettings &storageSettings)
{
    d->m_ui.cleanWhitespace->setChecked(storageSettings.m_cleanWhitespace);
    d->m_ui.inEntireDocument->setChecked(storageSettings.m_inEntireDocument);
    d->m_ui.cleanIndentation->setChecked(storageSettings.m_cleanIndentation);
    d->m_ui.addFinalNewLine->setChecked(storageSettings.m_addFinalNewLine);
}

void BehaviorSettingsWidget::assignedStorageSettings(StorageSettings *storageSettings) const
{
    storageSettings->m_cleanWhitespace = d->m_ui.cleanWhitespace->isChecked();
    storageSettings->m_inEntireDocument = d->m_ui.inEntireDocument->isChecked();
    storageSettings->m_cleanIndentation = d->m_ui.cleanIndentation->isChecked();
    storageSettings->m_addFinalNewLine = d->m_ui.addFinalNewLine->isChecked();
}

void BehaviorSettingsWidget::updateConstrainTooltipsBoxTooltip() const
{
    if (d->m_ui.constrainTooltipsBox->currentIndex() == 0) {
        d->m_ui.constrainTooltipsBox->setToolTip(
            tr("Displays context-sensitive help or type information on mouseover."));
    } else {
        d->m_ui.constrainTooltipsBox->setToolTip(
            tr("Displays context-sensitive help or type information on Shift+Mouseover."));
    }
}

void BehaviorSettingsWidget::setAssignedBehaviorSettings(const BehaviorSettings &behaviorSettings)
{
    d->m_ui.mouseHiding->setChecked(behaviorSettings.m_mouseHiding);
    d->m_ui.mouseNavigation->setChecked(behaviorSettings.m_mouseNavigation);
    d->m_ui.scrollWheelZooming->setChecked(behaviorSettings.m_scrollWheelZooming);
    d->m_ui.constrainTooltipsBox->setCurrentIndex(behaviorSettings.m_constrainHoverTooltips ? 1 : 0);
    d->m_ui.camelCaseNavigation->setChecked(behaviorSettings.m_camelCaseNavigation);
    d->m_ui.keyboardTooltips->setChecked(behaviorSettings.m_keyboardTooltips);
    d->m_ui.smartSelectionChanging->setChecked(behaviorSettings.m_smartSelectionChanging);
    updateConstrainTooltipsBoxTooltip();
}

void BehaviorSettingsWidget::assignedBehaviorSettings(BehaviorSettings *behaviorSettings) const
{
    behaviorSettings->m_mouseHiding = d->m_ui.mouseHiding->isChecked();
    behaviorSettings->m_mouseNavigation = d->m_ui.mouseNavigation->isChecked();
    behaviorSettings->m_scrollWheelZooming = d->m_ui.scrollWheelZooming->isChecked();
    behaviorSettings->m_constrainHoverTooltips = (d->m_ui.constrainTooltipsBox->currentIndex() == 1);
    behaviorSettings->m_camelCaseNavigation = d->m_ui.camelCaseNavigation->isChecked();
    behaviorSettings->m_keyboardTooltips = d->m_ui.keyboardTooltips->isChecked();
    behaviorSettings->m_smartSelectionChanging = d->m_ui.smartSelectionChanging->isChecked();
}

void BehaviorSettingsWidget::setAssignedExtraEncodingSettings(
    const ExtraEncodingSettings &encodingSettings)
{
    d->m_ui.utf8BomBox->setCurrentIndex(encodingSettings.m_utf8BomSetting);
}

void BehaviorSettingsWidget::assignedExtraEncodingSettings(
    ExtraEncodingSettings *encodingSettings) const
{
    encodingSettings->m_utf8BomSetting =
        (ExtraEncodingSettings::Utf8BomSetting)d->m_ui.utf8BomBox->currentIndex();
}

TabSettingsWidget *BehaviorSettingsWidget::tabSettingsWidget() const
{
    return d->m_ui.tabPreferencesWidget->tabSettingsWidget();
}

void BehaviorSettingsWidget::slotTypingSettingsChanged()
{
    TypingSettings settings;
    assignedTypingSettings(&settings);
    emit typingSettingsChanged(settings);
}

void BehaviorSettingsWidget::slotStorageSettingsChanged()
{
    StorageSettings settings;
    assignedStorageSettings(&settings);
    emit storageSettingsChanged(settings);
}

void BehaviorSettingsWidget::slotBehaviorSettingsChanged()
{
    BehaviorSettings settings;
    assignedBehaviorSettings(&settings);
    updateConstrainTooltipsBoxTooltip();
    emit behaviorSettingsChanged(settings);
}

void BehaviorSettingsWidget::slotExtraEncodingChanged()
{
    ExtraEncodingSettings settings;
    assignedExtraEncodingSettings(&settings);
    emit extraEncodingSettingsChanged(settings);
}

void BehaviorSettingsWidget::slotEncodingBoxChanged(int index)
{
    emit textCodecChanged(d->m_codecs.at(index));
}

} // TextEditor
