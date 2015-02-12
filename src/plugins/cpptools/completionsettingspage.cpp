/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "completionsettingspage.h"
#include "ui_completionsettingspage.h"

#include "cpptoolssettings.h"

#include <coreplugin/icore.h>
#include <texteditor/texteditorsettings.h>

#include <QTextStream>

using namespace CppTools;
using namespace CppTools::Internal;

CompletionSettingsPage::CompletionSettingsPage(QObject *parent)
    : TextEditor::TextEditorOptionsPage(parent)
    , m_page(0)
{
    setId("P.Completion");
    setDisplayName(tr("Completion"));
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

        const TextEditor::CompletionSettings &completionSettings =
                TextEditor::TextEditorSettings::completionSettings();

        int caseSensitivityIndex = 0;
        switch (completionSettings.m_caseSensitivity) {
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
        switch (completionSettings.m_completionTrigger) {
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
                    ->setValue(completionSettings.m_automaticProposalTimeoutInMs);
        m_page->autoInsertBrackets->setChecked(completionSettings.m_autoInsertBrackets);
        m_page->surroundSelectedText->setChecked(completionSettings.m_surroundingAutoBrackets);
        m_page->partiallyComplete->setChecked(completionSettings.m_partiallyComplete);
        m_page->spaceAfterFunctionName->setChecked(completionSettings.m_spaceAfterFunctionName);
        m_page->autoSplitStrings->setChecked(completionSettings.m_autoSplitStrings);

        const CommentsSettings &commentsSettings = CppToolsSettings::instance()->commentsSettings();
        m_page->enableDoxygenCheckBox->setChecked(commentsSettings.m_enableDoxygen);
        m_page->generateBriefCheckBox->setChecked(commentsSettings.m_generateBrief);
        m_page->leadingAsterisksCheckBox->setChecked(commentsSettings.m_leadingAsterisks);

        m_page->generateBriefCheckBox->setEnabled(m_page->enableDoxygenCheckBox->isChecked());
    }
    return m_widget;
}

void CompletionSettingsPage::apply()
{
    if (!m_page) // page was never shown
        return;

    TextEditor::CompletionSettings completionSettings;
    completionSettings.m_caseSensitivity = caseSensitivity();
    completionSettings.m_completionTrigger = completionTrigger();
    completionSettings.m_automaticProposalTimeoutInMs
            = m_page->automaticProposalTimeoutSpinBox->value();
    completionSettings.m_autoInsertBrackets = m_page->autoInsertBrackets->isChecked();
    completionSettings.m_surroundingAutoBrackets = m_page->surroundSelectedText->isChecked();
    completionSettings.m_partiallyComplete = m_page->partiallyComplete->isChecked();
    completionSettings.m_spaceAfterFunctionName = m_page->spaceAfterFunctionName->isChecked();
    completionSettings.m_autoSplitStrings = m_page->autoSplitStrings->isChecked();
    TextEditor::TextEditorSettings::setCompletionSettings(completionSettings);

    if (!requireCommentsSettingsUpdate())
        return;

    CommentsSettings commentsSettings;
    commentsSettings.m_enableDoxygen = m_page->enableDoxygenCheckBox->isChecked();
    commentsSettings.m_generateBrief = m_page->generateBriefCheckBox->isChecked();
    commentsSettings.m_leadingAsterisks = m_page->leadingAsterisksCheckBox->isChecked();
    CppToolsSettings::instance()->setCommentsSettings(commentsSettings);
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

bool CompletionSettingsPage::requireCommentsSettingsUpdate() const
{
    const CommentsSettings &commentsSettings = CppToolsSettings::instance()->commentsSettings();
    return commentsSettings.m_enableDoxygen != m_page->enableDoxygenCheckBox->isChecked()
        || commentsSettings.m_generateBrief != m_page->generateBriefCheckBox->isChecked()
        || commentsSettings.m_leadingAsterisks != m_page->leadingAsterisksCheckBox->isChecked();
}
