/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
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
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "completionsettingspage.h"

#include <ui_completionsettingspage.h>
#include "cpptoolsconstants.h"

#include <coreplugin/icore.h>
#include <texteditor/texteditorsettings.h>

#include <QTextStream>

using namespace CppTools;
using namespace CppTools::Internal;
using namespace CppTools::Constants;

CompletionSettingsPage::CompletionSettingsPage(QObject *parent)
    : TextEditor::TextEditorOptionsPage(parent)
    , m_page(0)
{
    m_commentsSettings.fromSettings(QLatin1String(CPPTOOLS_SETTINGSGROUP), Core::ICore::settings());

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

        const TextEditor::CompletionSettings &settings =
                TextEditor::TextEditorSettings::completionSettings();

        int caseSensitivityIndex = 0;
        switch (settings.m_caseSensitivity) {
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
        switch (settings.m_completionTrigger) {
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
        m_page->autoInsertBrackets->setChecked(settings.m_autoInsertBrackets);
        m_page->surroundSelectedText->setChecked(settings.m_surroundingAutoBrackets);
        m_page->partiallyComplete->setChecked(settings.m_partiallyComplete);
        m_page->spaceAfterFunctionName->setChecked(settings.m_spaceAfterFunctionName);
        m_page->autoSplitStrings->setChecked(settings.m_autoSplitStrings);
        m_page->enableDoxygenCheckBox->setChecked(m_commentsSettings.m_enableDoxygen);
        m_page->generateBriefCheckBox->setChecked(m_commentsSettings.m_generateBrief);
        m_page->leadingAsterisksCheckBox->setChecked(m_commentsSettings.m_leadingAsterisks);
        m_page->generateBriefCheckBox->setEnabled(m_page->enableDoxygenCheckBox->isChecked());
    }
    return m_widget;
}

void CompletionSettingsPage::apply()
{
    if (!m_page) // page was never shown
        return;
    TextEditor::CompletionSettings settings;
    settings.m_caseSensitivity = caseSensitivity();
    settings.m_completionTrigger = completionTrigger();
    settings.m_autoInsertBrackets = m_page->autoInsertBrackets->isChecked();
    settings.m_surroundingAutoBrackets = m_page->surroundSelectedText->isChecked();
    settings.m_partiallyComplete = m_page->partiallyComplete->isChecked();
    settings.m_spaceAfterFunctionName = m_page->spaceAfterFunctionName->isChecked();
    settings.m_autoSplitStrings = m_page->autoSplitStrings->isChecked();

    TextEditor::TextEditorSettings::setCompletionSettings(settings);

    if (!requireCommentsSettingsUpdate())
        return;

    m_commentsSettings.m_enableDoxygen = m_page->enableDoxygenCheckBox->isChecked();
    m_commentsSettings.m_generateBrief = m_page->generateBriefCheckBox->isChecked();
    m_commentsSettings.m_leadingAsterisks = m_page->leadingAsterisksCheckBox->isChecked();
    m_commentsSettings.toSettings(QLatin1String(CPPTOOLS_SETTINGSGROUP), Core::ICore::settings());

    emit commentsSettingsChanged(m_commentsSettings);
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

void CompletionSettingsPage::finish()
{
    delete m_widget;
    if (!m_page) // page was never shown
        return;
    delete m_page;
    m_page = 0;
}

const CommentsSettings &CompletionSettingsPage::commentsSettings() const
{
    return m_commentsSettings;
}

bool CompletionSettingsPage::requireCommentsSettingsUpdate() const
{
    return m_commentsSettings.m_enableDoxygen != m_page->enableDoxygenCheckBox->isChecked()
        || m_commentsSettings.m_generateBrief != m_page->generateBriefCheckBox->isChecked()
        || m_commentsSettings.m_leadingAsterisks != m_page->leadingAsterisksCheckBox->isChecked();
}
