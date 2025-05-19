// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmljscodestylepreferenceswidget.h"

#include "qmljscodestylesettings.h"
#include "qmljsformatterselectionwidget.h"
#include "qmljstoolstr.h"

#include <texteditor/simplecodestylepreferenceswidget.h>
#include <texteditor/texteditor.h>
#include <utils/layoutbuilder.h>
#include <utils/aspects.h>

#include <QLabel>
#include <QVBoxLayout>

using namespace TextEditor;
namespace QmlJSTools {

BuiltinFormatterSettingsWidget::BuiltinFormatterSettingsWidget(QWidget *parent, FormatterSelectionWidget *selection)
    : QmlCodeStyleWidgetBase(parent)
    , m_tabSettingsWidget(new TextEditor::TabSettingsWidget)
    , m_formatterSelectionWidget(selection)
{
    m_lineLength.setRange(0, 999);
    m_tabSettingsWidget->setParent(this);

    using namespace Layouting;
    Column{Group{
               title(Tr::tr("Built-in Formatter Settings")),
               Column{
                   m_tabSettingsWidget,
                   Group{
                       title(Tr::tr("Other Settings")),
                       Form{Tr::tr("Line length:"), m_lineLength, br}}}}}
        .attachTo(this);

    connect(
        &m_lineLength,
        &Utils::IntegerAspect::changed,
        this,
        &BuiltinFormatterSettingsWidget::slotSettingsChanged);
}

void BuiltinFormatterSettingsWidget::setCodeStyleSettings(const QmlJSCodeStyleSettings &settings)
{
    QSignalBlocker blocker(this);
    m_lineLength.setValue(settings.lineLength);
}

void BuiltinFormatterSettingsWidget::setPreferences(QmlJSCodeStylePreferences *preferences)
{
    if (m_preferences == preferences)
        return; // nothing changes

    slotCurrentPreferencesChanged(preferences);

    // cleanup old
    if (m_preferences) {
        disconnect(m_preferences, &QmlJSCodeStylePreferences::currentValueChanged, this, nullptr);
        disconnect(m_preferences, &QmlJSCodeStylePreferences::currentPreferencesChanged,
                   this, &BuiltinFormatterSettingsWidget::slotCurrentPreferencesChanged);
        disconnect(m_preferences, &ICodeStylePreferences::currentTabSettingsChanged,
                    m_tabSettingsWidget, &TabSettingsWidget::setTabSettings);
        disconnect(m_tabSettingsWidget, &TabSettingsWidget::settingsChanged,
                    this, &BuiltinFormatterSettingsWidget::slotTabSettingsChanged);
    }
    m_preferences = preferences;
    // fillup new
    if (m_preferences) {
        setCodeStyleSettings(m_preferences->currentCodeStyleSettings());
        connect(m_preferences, &QmlJSCodeStylePreferences::currentValueChanged, this, [this] {
            setCodeStyleSettings(m_preferences->currentCodeStyleSettings());
        });
        connect(m_preferences, &QmlJSCodeStylePreferences::currentPreferencesChanged,
                this, &BuiltinFormatterSettingsWidget::slotCurrentPreferencesChanged);
                m_tabSettingsWidget->setTabSettings(m_preferences->currentTabSettings());
        connect(m_preferences, &ICodeStylePreferences::currentTabSettingsChanged,
                m_tabSettingsWidget, &TabSettingsWidget::setTabSettings);
        connect(m_tabSettingsWidget, &TabSettingsWidget::settingsChanged,
                this, &BuiltinFormatterSettingsWidget::slotTabSettingsChanged);
    }
}

void BuiltinFormatterSettingsWidget::slotCurrentPreferencesChanged(
    TextEditor::ICodeStylePreferences *preferences)
{
    QmlJSCodeStylePreferences *current = dynamic_cast<QmlJSCodeStylePreferences *>(
        preferences ? preferences->currentPreferences() : nullptr);
    const bool enableWidgets = current && !current->isReadOnly() && m_formatterSelectionWidget
                               && m_formatterSelectionWidget->selection().value()
                                      == QmlCodeStyleWidgetBase::Builtin;
    setEnabled(enableWidgets);
}

void BuiltinFormatterSettingsWidget::slotSettingsChanged()
{
    QmlJSCodeStyleSettings settings = m_preferences ? m_preferences->currentCodeStyleSettings()
                                               : QmlJSCodeStyleSettings::currentGlobalCodeStyle();
    settings.lineLength = m_lineLength.value();
    emit settingsChanged(settings);
}

void BuiltinFormatterSettingsWidget::slotTabSettingsChanged(const TextEditor::TabSettings &settings)
{
    if (!m_preferences)
        return;

    ICodeStylePreferences *current = m_preferences->currentPreferences();
    if (!current)
        return;

    current->setTabSettings(settings);
}

} // namespace QmlJSTools
