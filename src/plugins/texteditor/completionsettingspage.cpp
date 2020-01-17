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

#include "completionsettingspage.h"

#include "texteditorsettings.h"
#include "texteditorconstants.h"
#include "ui_completionsettingspage.h"

#include <cpptools/cpptoolssettings.h>

#include <coreplugin/icore.h>

#include <QTextStream>

using namespace CppTools;

namespace TextEditor {
namespace Internal {

class CompletionSettingsPageWidget final : public Core::IOptionsPageWidget
{
    Q_DECLARE_TR_FUNCTIONS(TextEditor::Internal::CompletionSettingsPage)

public:
    explicit CompletionSettingsPageWidget(CompletionSettingsPage *owner);

private:
    void apply() final;

    CaseSensitivity caseSensitivity() const;
    CompletionTrigger completionTrigger() const;
    void settingsFromUi(CompletionSettings &completion, CommentsSettings &comment) const;

    CompletionSettingsPage *m_owner = nullptr;
    Ui::CompletionSettingsPage m_ui;
};

CompletionSettingsPageWidget::CompletionSettingsPageWidget(CompletionSettingsPage *owner)
    : m_owner(owner)
{
    m_ui.setupUi(this);

    connect(m_ui.completionTrigger, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this] {
        const bool enableTimeoutWidgets = completionTrigger() == AutomaticCompletion;
        m_ui.automaticProposalTimeoutLabel->setEnabled(enableTimeoutWidgets);
        m_ui.automaticProposalTimeoutSpinBox->setEnabled(enableTimeoutWidgets);
    });

    int caseSensitivityIndex = 0;
    switch (m_owner->m_completionSettings.m_caseSensitivity) {
    case TextEditor::CaseSensitive:
        caseSensitivityIndex = 0;
        break;
    case TextEditor::CaseInsensitive:
        caseSensitivityIndex = 1;
        break;
    case TextEditor::FirstLetterCaseSensitive:
        caseSensitivityIndex = 2;
        break;
    }

    int completionTriggerIndex = 0;
    switch (m_owner->m_completionSettings.m_completionTrigger) {
    case TextEditor::ManualCompletion:
        completionTriggerIndex = 0;
        break;
    case TextEditor::TriggeredCompletion:
        completionTriggerIndex = 1;
        break;
    case TextEditor::AutomaticCompletion:
        completionTriggerIndex = 2;
        break;
    }

    m_ui.caseSensitivity->setCurrentIndex(caseSensitivityIndex);
    m_ui.completionTrigger->setCurrentIndex(completionTriggerIndex);
    m_ui.automaticProposalTimeoutSpinBox
            ->setValue(m_owner->m_completionSettings.m_automaticProposalTimeoutInMs);
    m_ui.insertBrackets->setChecked(m_owner->m_completionSettings.m_autoInsertBrackets);
    m_ui.surroundBrackets->setChecked(m_owner->m_completionSettings.m_surroundingAutoBrackets);
    m_ui.insertQuotes->setChecked(m_owner->m_completionSettings.m_autoInsertQuotes);
    m_ui.surroundQuotes->setChecked(m_owner->m_completionSettings.m_surroundingAutoQuotes);
    m_ui.partiallyComplete->setChecked(m_owner->m_completionSettings.m_partiallyComplete);
    m_ui.spaceAfterFunctionName->setChecked(m_owner->m_completionSettings.m_spaceAfterFunctionName);
    m_ui.autoSplitStrings->setChecked(m_owner->m_completionSettings.m_autoSplitStrings);
    m_ui.animateAutoComplete->setChecked(m_owner->m_completionSettings.m_animateAutoComplete);
    m_ui.overwriteClosingChars->setChecked(m_owner->m_completionSettings.m_overwriteClosingChars);
    m_ui.highlightAutoComplete->setChecked(m_owner->m_completionSettings.m_highlightAutoComplete);
    m_ui.skipAutoComplete->setChecked(m_owner->m_completionSettings.m_skipAutoCompletedText);
    m_ui.removeAutoComplete->setChecked(m_owner->m_completionSettings.m_autoRemove);

    m_ui.enableDoxygenCheckBox->setChecked(m_owner->m_commentsSettings.m_enableDoxygen);
    m_ui.generateBriefCheckBox->setChecked(m_owner->m_commentsSettings.m_generateBrief);
    m_ui.leadingAsterisksCheckBox->setChecked(m_owner->m_commentsSettings.m_leadingAsterisks);

    m_ui.generateBriefCheckBox->setEnabled(m_ui.enableDoxygenCheckBox->isChecked());
    m_ui.skipAutoComplete->setEnabled(m_ui.highlightAutoComplete->isChecked());
    m_ui.removeAutoComplete->setEnabled(m_ui.highlightAutoComplete->isChecked());
}

void CompletionSettingsPageWidget::apply()
{
    CompletionSettings completionSettings;
    CommentsSettings commentsSettings;

    settingsFromUi(completionSettings, commentsSettings);

    if (m_owner->m_completionSettings != completionSettings) {
        m_owner->m_completionSettings = completionSettings;
        m_owner->m_completionSettings.toSettings(Core::ICore::settings());
        emit TextEditorSettings::instance()->completionSettingsChanged(completionSettings);
    }

    if (m_owner->m_commentsSettings != commentsSettings) {
        m_owner->m_commentsSettings = commentsSettings;
        m_owner->m_commentsSettings.toSettings(Core::ICore::settings());
        emit TextEditorSettings::instance()->commentsSettingsChanged(commentsSettings);
    }
}

CaseSensitivity CompletionSettingsPageWidget::caseSensitivity() const
{
    switch (m_ui.caseSensitivity->currentIndex()) {
    case 0: // Full
        return TextEditor::CaseSensitive;
    case 1: // None
        return TextEditor::CaseInsensitive;
    default: // First letter
        return TextEditor::FirstLetterCaseSensitive;
    }
}

CompletionTrigger CompletionSettingsPageWidget::completionTrigger() const
{
    switch (m_ui.completionTrigger->currentIndex()) {
    case 0:
        return TextEditor::ManualCompletion;
    case 1:
        return TextEditor::TriggeredCompletion;
    default:
        return TextEditor::AutomaticCompletion;
    }
}

void CompletionSettingsPageWidget::settingsFromUi(CompletionSettings &completion,
                                                  CommentsSettings &comment) const
{
    completion.m_caseSensitivity = caseSensitivity();
    completion.m_completionTrigger = completionTrigger();
    completion.m_automaticProposalTimeoutInMs
            = m_ui.automaticProposalTimeoutSpinBox->value();
    completion.m_autoInsertBrackets = m_ui.insertBrackets->isChecked();
    completion.m_surroundingAutoBrackets = m_ui.surroundBrackets->isChecked();
    completion.m_autoInsertQuotes = m_ui.insertQuotes->isChecked();
    completion.m_surroundingAutoQuotes = m_ui.surroundQuotes->isChecked();
    completion.m_partiallyComplete = m_ui.partiallyComplete->isChecked();
    completion.m_spaceAfterFunctionName = m_ui.spaceAfterFunctionName->isChecked();
    completion.m_autoSplitStrings = m_ui.autoSplitStrings->isChecked();
    completion.m_animateAutoComplete = m_ui.animateAutoComplete->isChecked();
    completion.m_overwriteClosingChars = m_ui.overwriteClosingChars->isChecked();
    completion.m_highlightAutoComplete = m_ui.highlightAutoComplete->isChecked();
    completion.m_skipAutoCompletedText = m_ui.skipAutoComplete->isChecked();
    completion.m_autoRemove = m_ui.removeAutoComplete->isChecked();

    comment.m_enableDoxygen = m_ui.enableDoxygenCheckBox->isChecked();
    comment.m_generateBrief = m_ui.generateBriefCheckBox->isChecked();
    comment.m_leadingAsterisks = m_ui.leadingAsterisksCheckBox->isChecked();
}

const CompletionSettings &CompletionSettingsPage::completionSettings()
{
    return m_completionSettings;
}

const CommentsSettings &CompletionSettingsPage::commentsSettings()
{
    return m_commentsSettings;
}

CompletionSettingsPage::CompletionSettingsPage()
{
    setId("P.Completion");
    setDisplayName(CompletionSettingsPageWidget::tr("Completion"));
    setCategory(TextEditor::Constants::TEXT_EDITOR_SETTINGS_CATEGORY);
    setDisplayCategory(QCoreApplication::translate("TextEditor", "Text Editor"));
    setCategoryIconPath(TextEditor::Constants::TEXT_EDITOR_SETTINGS_CATEGORY_ICON_PATH);
    setWidgetCreator([this] { return new CompletionSettingsPageWidget(this); });

    QSettings *s = Core::ICore::settings();
    m_completionSettings.fromSettings(s);
    m_commentsSettings.fromSettings(s);
}

} // Internal
} // TextEditor
