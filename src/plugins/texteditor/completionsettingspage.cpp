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
#include "ui_completionsettingspage.h"
#include "texteditorsettings.h"

#include <cpptools/cpptoolssettings.h>

#include <coreplugin/icore.h>

#include <QTextStream>

using namespace TextEditor;
using namespace TextEditor::Internal;
using namespace CppTools;

CompletionSettingsPage::CompletionSettingsPage(QObject *parent)
    : TextEditor::TextEditorOptionsPage(parent)
    , m_page(0)
{
    setId("P.Completion");
    setDisplayName(tr("Completion"));

    QSettings *s = Core::ICore::settings();
    m_completionSettings.fromSettings(s);
    m_commentsSettings.fromSettings(s);
}

CompletionSettingsPage::~CompletionSettingsPage()
{
    delete m_page;
}

QWidget *CompletionSettingsPage::widget()
{
    if (!m_widget) {
        m_widget = new QWidget;
        m_page = new Ui::CompletionSettingsPage;
        m_page->setupUi(m_widget);

        connect(m_page->completionTrigger,
                static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
                this, &CompletionSettingsPage::onCompletionTriggerChanged);

        int caseSensitivityIndex = 0;
        switch (m_completionSettings.m_caseSensitivity) {
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
        switch (m_completionSettings.m_completionTrigger) {
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

        m_page->caseSensitivity->setCurrentIndex(caseSensitivityIndex);
        m_page->completionTrigger->setCurrentIndex(completionTriggerIndex);
        m_page->automaticProposalTimeoutSpinBox
                    ->setValue(m_completionSettings.m_automaticProposalTimeoutInMs);
        m_page->insertBrackets->setChecked(m_completionSettings.m_autoInsertBrackets);
        m_page->surroundBrackets->setChecked(m_completionSettings.m_surroundingAutoBrackets);
        m_page->insertQuotes->setChecked(m_completionSettings.m_autoInsertQuotes);
        m_page->surroundQuotes->setChecked(m_completionSettings.m_surroundingAutoQuotes);
        m_page->partiallyComplete->setChecked(m_completionSettings.m_partiallyComplete);
        m_page->spaceAfterFunctionName->setChecked(m_completionSettings.m_spaceAfterFunctionName);
        m_page->autoSplitStrings->setChecked(m_completionSettings.m_autoSplitStrings);
        m_page->animateAutoComplete->setChecked(m_completionSettings.m_animateAutoComplete);
        m_page->highlightAutoComplete->setChecked(m_completionSettings.m_highlightAutoComplete);
        m_page->skipAutoComplete->setChecked(m_completionSettings.m_skipAutoCompletedText);
        m_page->removeAutoComplete->setChecked(m_completionSettings.m_autoRemove);

        m_page->enableDoxygenCheckBox->setChecked(m_commentsSettings.m_enableDoxygen);
        m_page->generateBriefCheckBox->setChecked(m_commentsSettings.m_generateBrief);
        m_page->leadingAsterisksCheckBox->setChecked(m_commentsSettings.m_leadingAsterisks);

        m_page->generateBriefCheckBox->setEnabled(m_page->enableDoxygenCheckBox->isChecked());
        m_page->skipAutoComplete->setEnabled(m_page->highlightAutoComplete->isChecked());
        m_page->removeAutoComplete->setEnabled(m_page->highlightAutoComplete->isChecked());
    }
    return m_widget;
}

void CompletionSettingsPage::apply()
{
    if (!m_page) // page was never shown
        return;

    CompletionSettings completionSettings;
    CommentsSettings commentsSettings;

    settingsFromUi(completionSettings, commentsSettings);

    if (m_completionSettings != completionSettings) {
        m_completionSettings = completionSettings;
        m_completionSettings.toSettings(Core::ICore::settings());
        emit completionSettingsChanged(completionSettings);
    }

    if (m_commentsSettings != commentsSettings) {
        m_commentsSettings = commentsSettings;
        m_commentsSettings.toSettings(Core::ICore::settings());
        emit commentsSettingsChanged(commentsSettings);
    }
}

TextEditor::CaseSensitivity CompletionSettingsPage::caseSensitivity() const
{
    switch (m_page->caseSensitivity->currentIndex()) {
    case 0: // Full
        return TextEditor::CaseSensitive;
    case 1: // None
        return TextEditor::CaseInsensitive;
    default: // First letter
        return TextEditor::FirstLetterCaseSensitive;
    }
}

TextEditor::CompletionTrigger CompletionSettingsPage::completionTrigger() const
{
    switch (m_page->completionTrigger->currentIndex()) {
    case 0:
        return TextEditor::ManualCompletion;
    case 1:
        return TextEditor::TriggeredCompletion;
    default:
        return TextEditor::AutomaticCompletion;
    }
}

void CompletionSettingsPage::settingsFromUi(CompletionSettings &completion, CommentsSettings &comment) const
{
    if (!m_page)
        return;

    completion.m_caseSensitivity = caseSensitivity();
    completion.m_completionTrigger = completionTrigger();
    completion.m_automaticProposalTimeoutInMs
            = m_page->automaticProposalTimeoutSpinBox->value();
    completion.m_autoInsertBrackets = m_page->insertBrackets->isChecked();
    completion.m_surroundingAutoBrackets = m_page->surroundBrackets->isChecked();
    completion.m_autoInsertQuotes = m_page->insertQuotes->isChecked();
    completion.m_surroundingAutoQuotes = m_page->surroundQuotes->isChecked();
    completion.m_partiallyComplete = m_page->partiallyComplete->isChecked();
    completion.m_spaceAfterFunctionName = m_page->spaceAfterFunctionName->isChecked();
    completion.m_autoSplitStrings = m_page->autoSplitStrings->isChecked();
    completion.m_animateAutoComplete = m_page->animateAutoComplete->isChecked();
    completion.m_highlightAutoComplete = m_page->highlightAutoComplete->isChecked();
    completion.m_skipAutoCompletedText = m_page->skipAutoComplete->isChecked();
    completion.m_autoRemove = m_page->removeAutoComplete->isChecked();

    comment.m_enableDoxygen = m_page->enableDoxygenCheckBox->isChecked();
    comment.m_generateBrief = m_page->generateBriefCheckBox->isChecked();
    comment.m_leadingAsterisks = m_page->leadingAsterisksCheckBox->isChecked();
}

void CompletionSettingsPage::onCompletionTriggerChanged()
{
    const bool enableTimeoutWidgets = completionTrigger() == TextEditor::AutomaticCompletion;
    m_page->automaticProposalTimeoutLabel->setEnabled(enableTimeoutWidgets);
    m_page->automaticProposalTimeoutSpinBox->setEnabled(enableTimeoutWidgets);
}

void CompletionSettingsPage::finish()
{
    delete m_widget;
    if (!m_page) // page was never shown
        return;
    delete m_page;
    m_page = 0;
}

const CompletionSettings &CompletionSettingsPage::completionSettings()
{
    return m_completionSettings;
}

const CommentsSettings &CompletionSettingsPage::commentsSettings()
{
    return m_commentsSettings;
}
