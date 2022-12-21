// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "completionsettingspage.h"

#include "texteditorsettings.h"
#include "texteditorconstants.h"

#include <cppeditor/cpptoolssettings.h>

#include <coreplugin/icore.h>

#include <utils/layoutbuilder.h>

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QGroupBox>
#include <QLabel>
#include <QSpacerItem>
#include <QSpinBox>
#include <QWidget>

using namespace CppEditor;
using namespace Utils;

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

    QComboBox *m_caseSensitivity;
    QComboBox *m_completionTrigger;
    QSpinBox *m_thresholdSpinBox;
    QSpinBox *m_automaticProposalTimeoutSpinBox;
    QCheckBox *m_partiallyComplete;
    QCheckBox *m_autoSplitStrings;
    QCheckBox *m_insertBrackets;
    QCheckBox *m_insertQuotes;
    QCheckBox *m_surroundBrackets;
    QCheckBox *m_spaceAfterFunctionName;
    QCheckBox *m_surroundQuotes;
    QCheckBox *m_animateAutoComplete;
    QCheckBox *m_highlightAutoComplete;
    QCheckBox *m_skipAutoComplete;
    QCheckBox *m_removeAutoComplete;
    QCheckBox *m_overwriteClosingChars;
    QCheckBox *m_enableDoxygenCheckBox;
    QCheckBox *m_generateBriefCheckBox;
    QCheckBox *m_leadingAsterisksCheckBox;
};

CompletionSettingsPageWidget::CompletionSettingsPageWidget(CompletionSettingsPage *owner)
    : m_owner(owner)
{
    resize(823, 756);

    m_caseSensitivity = new QComboBox;
    m_caseSensitivity->addItem(tr("Full"));
    m_caseSensitivity->addItem(tr("None"));
    m_caseSensitivity->addItem(tr("First Letter"));

    auto caseSensitivityLabel = new QLabel(tr("&Case-sensitivity:"));
    caseSensitivityLabel->setBuddy(m_caseSensitivity);

    m_completionTrigger = new QComboBox;
    m_completionTrigger->addItem(tr("Manually"));
    m_completionTrigger->addItem(tr("When Triggered"));
    m_completionTrigger->addItem(tr("Always"));

    auto completionTriggerLabel = new QLabel(tr("Activate completion:"));

    auto automaticProposalTimeoutLabel = new QLabel(tr("Timeout in ms:"));

    m_automaticProposalTimeoutSpinBox = new QSpinBox;
    m_automaticProposalTimeoutSpinBox->setMaximum(2000);
    m_automaticProposalTimeoutSpinBox->setSingleStep(50);
    m_automaticProposalTimeoutSpinBox->setValue(400);

    auto thresholdLabel = new QLabel(tr("Character threshold:"));

    m_thresholdSpinBox = new QSpinBox;
    m_thresholdSpinBox->setMinimum(1);

    m_partiallyComplete = new QCheckBox(tr("Autocomplete common &prefix"));
    m_partiallyComplete->setToolTip(tr("Inserts the common prefix of available completion items."));
    m_partiallyComplete->setChecked(true);

    m_autoSplitStrings = new QCheckBox(tr("Automatically split strings"));
    m_autoSplitStrings->setToolTip(
        tr("Splits a string into two lines by adding an end quote at the cursor position "
           "when you press Enter and a start quote to the next line, before the rest "
           "of the string.\n\n"
           "In addition, Shift+Enter inserts an escape character at the cursor position "
           "and moves the rest of the string to the next line."));

    m_insertBrackets = new QCheckBox(tr("Insert opening or closing brackets"));
    m_insertBrackets->setChecked(true);

    m_insertQuotes = new QCheckBox(tr("Insert closing quote"));
    m_insertQuotes->setChecked(true);

    m_surroundBrackets = new QCheckBox(tr("Surround text selection with brackets"));
    m_surroundBrackets->setChecked(true);
    m_surroundBrackets->setToolTip(
        tr("When typing a matching bracket and there is a text selection, instead of "
           "removing the selection, surrounds it with the corresponding characters."));

    m_spaceAfterFunctionName = new QCheckBox(tr("Insert &space after function name"));
    m_spaceAfterFunctionName->setEnabled(true);

    m_surroundQuotes = new QCheckBox(tr("Surround text selection with quotes"));
    m_surroundQuotes->setChecked(true);
    m_surroundQuotes->setToolTip(
        tr("When typing a matching quote and there is a text selection, instead of "
           "removing the selection, surrounds it with the corresponding characters."));

    m_animateAutoComplete = new QCheckBox(tr("Animate automatically inserted text"));
    m_animateAutoComplete->setChecked(true);
    m_animateAutoComplete->setToolTip(tr("Show a visual hint when for example a brace or a quote "
                                       "is automatically inserted by the editor."));

    m_highlightAutoComplete = new QCheckBox(tr("Highlight automatically inserted text"));
    m_highlightAutoComplete->setChecked(true);

    m_skipAutoComplete = new QCheckBox(tr("Skip automatically inserted character when typing"));
    m_skipAutoComplete->setToolTip(tr("Skip automatically inserted character if re-typed manually "
                                    "after completion or by pressing tab."));
    m_skipAutoComplete->setChecked(true);

    m_removeAutoComplete = new QCheckBox(tr("Remove automatically inserted text on backspace"));
    m_removeAutoComplete->setChecked(true);
    m_removeAutoComplete->setToolTip(tr("Remove the automatically inserted character if the trigger "
                                      "is deleted by backspace after the completion."));

    m_overwriteClosingChars = new QCheckBox(tr("Overwrite closing punctuation"));
    m_overwriteClosingChars->setToolTip(tr("Automatically overwrite closing parentheses and quotes."));

    m_enableDoxygenCheckBox = new QCheckBox(tr("Enable Doxygen blocks"));
    m_enableDoxygenCheckBox->setToolTip(tr("Automatically creates a Doxygen comment upon pressing "
                                         "enter after a '/**', '/*!', '//!' or '///'."));

    m_generateBriefCheckBox = new QCheckBox(tr("Generate brief description"));
    m_generateBriefCheckBox->setToolTip(tr("Generates a <i>brief</i> command with an initial "
                                         "description for the corresponding declaration."));

    m_leadingAsterisksCheckBox = new QCheckBox(tr("Add leading asterisks"));
    m_leadingAsterisksCheckBox->setToolTip(
        tr("Adds leading asterisks when continuing C/C++ \"/*\", Qt \"/*!\" "
           "and Java \"/**\" style comments on new lines."));

    connect(m_completionTrigger, &QComboBox::currentIndexChanged,
            this, [this, automaticProposalTimeoutLabel] {
        const bool enableTimeoutWidgets = completionTrigger() == AutomaticCompletion;
        automaticProposalTimeoutLabel->setEnabled(enableTimeoutWidgets);
        m_automaticProposalTimeoutSpinBox->setEnabled(enableTimeoutWidgets);
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

    m_caseSensitivity->setCurrentIndex(caseSensitivityIndex);
    m_completionTrigger->setCurrentIndex(completionTriggerIndex);
    m_automaticProposalTimeoutSpinBox
            ->setValue(m_owner->m_completionSettings.m_automaticProposalTimeoutInMs);
    m_thresholdSpinBox->setValue(m_owner->m_completionSettings.m_characterThreshold);
    m_insertBrackets->setChecked(m_owner->m_completionSettings.m_autoInsertBrackets);
    m_surroundBrackets->setChecked(m_owner->m_completionSettings.m_surroundingAutoBrackets);
    m_insertQuotes->setChecked(m_owner->m_completionSettings.m_autoInsertQuotes);
    m_surroundQuotes->setChecked(m_owner->m_completionSettings.m_surroundingAutoQuotes);
    m_partiallyComplete->setChecked(m_owner->m_completionSettings.m_partiallyComplete);
    m_spaceAfterFunctionName->setChecked(m_owner->m_completionSettings.m_spaceAfterFunctionName);
    m_autoSplitStrings->setChecked(m_owner->m_completionSettings.m_autoSplitStrings);
    m_animateAutoComplete->setChecked(m_owner->m_completionSettings.m_animateAutoComplete);
    m_overwriteClosingChars->setChecked(m_owner->m_completionSettings.m_overwriteClosingChars);
    m_highlightAutoComplete->setChecked(m_owner->m_completionSettings.m_highlightAutoComplete);
    m_skipAutoComplete->setChecked(m_owner->m_completionSettings.m_skipAutoCompletedText);
    m_removeAutoComplete->setChecked(m_owner->m_completionSettings.m_autoRemove);

    m_enableDoxygenCheckBox->setChecked(m_owner->m_commentsSettings.m_enableDoxygen);
    m_generateBriefCheckBox->setChecked(m_owner->m_commentsSettings.m_generateBrief);
    m_leadingAsterisksCheckBox->setChecked(m_owner->m_commentsSettings.m_leadingAsterisks);

    m_generateBriefCheckBox->setEnabled(m_enableDoxygenCheckBox->isChecked());
    m_skipAutoComplete->setEnabled(m_highlightAutoComplete->isChecked());
    m_removeAutoComplete->setEnabled(m_highlightAutoComplete->isChecked());

    using namespace Layouting;
    auto indent = [](QWidget *widget) { return Row { Space(30), widget }; };

    Column {
        Group {
            title(tr("Behavior")),
            Form {
                caseSensitivityLabel, m_caseSensitivity, st, br,
                completionTriggerLabel, m_completionTrigger, st, br,
                automaticProposalTimeoutLabel, m_automaticProposalTimeoutSpinBox, st, br,
                thresholdLabel, m_thresholdSpinBox, st, br,
                Span(2, m_partiallyComplete), br,
                Span(2, m_autoSplitStrings), br,
            }
        },
        Group {
            title(tr("&Automatically insert matching characters")),
            Row {
                Column {
                    m_insertBrackets,
                    m_surroundBrackets,
                    m_spaceAfterFunctionName,
                    m_highlightAutoComplete,
                        indent(m_skipAutoComplete),
                        indent(m_removeAutoComplete)
                },
                Column {
                    m_insertQuotes,
                    m_surroundQuotes,
                    m_animateAutoComplete,
                    m_overwriteClosingChars,
                    st,
                }
            }
        },
        Group {
            title(tr("Documentation Comments")),
            Column {
                m_enableDoxygenCheckBox,
                    indent(m_generateBriefCheckBox),
                m_leadingAsterisksCheckBox
            }
        },
        st
    }.attachTo(this);

    connect(m_enableDoxygenCheckBox, &QCheckBox::toggled, m_generateBriefCheckBox, &QCheckBox::setEnabled);
    connect(m_highlightAutoComplete, &QCheckBox::toggled, m_skipAutoComplete, &QCheckBox::setEnabled);
    connect(m_highlightAutoComplete, &QCheckBox::toggled, m_removeAutoComplete, &QCheckBox::setEnabled);
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
    switch (m_caseSensitivity->currentIndex()) {
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
    switch (m_completionTrigger->currentIndex()) {
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
            = m_automaticProposalTimeoutSpinBox->value();
    completion.m_characterThreshold = m_thresholdSpinBox->value();
    completion.m_autoInsertBrackets = m_insertBrackets->isChecked();
    completion.m_surroundingAutoBrackets = m_surroundBrackets->isChecked();
    completion.m_autoInsertQuotes = m_insertQuotes->isChecked();
    completion.m_surroundingAutoQuotes = m_surroundQuotes->isChecked();
    completion.m_partiallyComplete = m_partiallyComplete->isChecked();
    completion.m_spaceAfterFunctionName = m_spaceAfterFunctionName->isChecked();
    completion.m_autoSplitStrings = m_autoSplitStrings->isChecked();
    completion.m_animateAutoComplete = m_animateAutoComplete->isChecked();
    completion.m_overwriteClosingChars = m_overwriteClosingChars->isChecked();
    completion.m_highlightAutoComplete = m_highlightAutoComplete->isChecked();
    completion.m_skipAutoCompletedText = m_skipAutoComplete->isChecked();
    completion.m_autoRemove = m_removeAutoComplete->isChecked();

    comment.m_enableDoxygen = m_enableDoxygenCheckBox->isChecked();
    comment.m_generateBrief = m_generateBriefCheckBox->isChecked();
    comment.m_leadingAsterisks = m_leadingAsterisksCheckBox->isChecked();
}

const CompletionSettings &CompletionSettingsPage::completionSettings() const
{
    return m_completionSettings;
}

const CommentsSettings &CompletionSettingsPage::commentsSettings() const
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
