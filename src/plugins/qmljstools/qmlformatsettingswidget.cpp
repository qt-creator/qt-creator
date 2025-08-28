// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmlformatsettingswidget.h"
#include "qmljsformatterselectionwidget.h"
#include "qmlformatsettings.h"
#include "qmljstoolstr.h"

#include <texteditor/snippets/snippeteditor.h>
#include <utils/layoutbuilder.h>

#include <QVBoxLayout>

namespace QmlJSTools {

QmlFormatSettingsWidget::QmlFormatSettingsWidget(
    QWidget *parent, FormatterSelectionWidget *selection)
    : QmlCodeStyleWidgetBase(parent)
    , m_qmlformatConfigTextEdit(std::make_unique<TextEditor::SnippetEditorWidget>())
    , m_formatterSelectionWidget(selection)
{
    QSizePolicy sp(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    sp.setHorizontalStretch(1);
    m_qmlformatConfigTextEdit->setSizePolicy(sp);

    using namespace Layouting;
    // clang-format off
    Column {
        Group {
            title(Tr::tr("Global qmlformat Configuration")),
            Column {
                m_qmlformatConfigTextEdit.get(),
            },
        },
        noMargin
    }.attachTo(this);
    // clang-format on

    connect(
        m_qmlformatConfigTextEdit.get(),
        &TextEditor::SnippetEditorWidget::textChanged,
        this,
        &QmlFormatSettingsWidget::slotSettingsChanged);
}

void QmlFormatSettingsWidget::setCodeStyleSettings(const QmlJSCodeStyleSettings &s)
{
    QSignalBlocker blocker(this);
    if (s.qmlformatIniContent != m_qmlformatConfigTextEdit->toPlainText())
        m_qmlformatConfigTextEdit->setPlainText(s.qmlformatIniContent);
}

void QmlFormatSettingsWidget::setPreferences(QmlJSCodeStylePreferences *preferences)
{
    if (m_preferences == preferences)
        return; // nothing changes

    slotCurrentPreferencesChanged(preferences);

    // cleanup old
    if (m_preferences) {
        disconnect(m_preferences, &QmlJSCodeStylePreferences::currentValueChanged, this, nullptr);
        disconnect(m_preferences, &QmlJSCodeStylePreferences::currentPreferencesChanged,
                   this, &QmlFormatSettingsWidget::slotCurrentPreferencesChanged);
    }
    m_preferences = preferences;
    // fillup new
    if (m_preferences) {
        setCodeStyleSettings(m_preferences->currentCodeStyleSettings());
        connect(m_preferences, &QmlJSCodeStylePreferences::currentValueChanged, this, [this] {
                this->setCodeStyleSettings(m_preferences->currentCodeStyleSettings());
        });
        connect(m_preferences, &QmlJSCodeStylePreferences::currentPreferencesChanged,
                this, &QmlFormatSettingsWidget::slotCurrentPreferencesChanged);
    }
}

void QmlFormatSettingsWidget::slotCurrentPreferencesChanged(
    TextEditor::ICodeStylePreferences *preferences)
{
    auto *current = dynamic_cast<QmlJSCodeStylePreferences *>(
        preferences ? preferences->currentPreferences() : nullptr);
    const bool enableWidgets = current && !current->isReadOnly() && m_formatterSelectionWidget
                               && m_formatterSelectionWidget->selection().value()
                                      == QmlCodeStyleWidgetBase::QmlFormat;
    setEnabled(enableWidgets);
}

void QmlFormatSettingsWidget::slotSettingsChanged()
{
    QmlJSCodeStyleSettings settings = m_preferences ? m_preferences->currentCodeStyleSettings()
    : QmlJSCodeStyleSettings::currentGlobalCodeStyle();
    settings.qmlformatIniContent = m_qmlformatConfigTextEdit->toPlainText();
    QmlFormatSettings::instance().globalQmlFormatIniFile().writeFileContents(settings.qmlformatIniContent.toUtf8());
    emit settingsChanged(settings);
}

} // namespace QmlJSTools
