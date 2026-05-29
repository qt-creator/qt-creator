// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "completionsettings.h"

#include "texteditorconstants.h"
#include "texteditortr.h"

#include <cppeditor/cpptoolssettings.h>

#include <coreplugin/dialogs/ioptionspage.h>
#include <coreplugin/icore.h>

#include <utils/layoutbuilder.h>

using namespace CppEditor;
using namespace Utils;

namespace TextEditor {

CompletionSettings &globalCompletionSettings()
{
    static CompletionSettings theCompletionSettings;
    return theCompletionSettings;
}

CompletionSettings::CompletionSettings()
{
    setAutoApply(false);
    setSettingsGroup("CppTools/Completion");

    caseSensitivity.setSettingsKey("CaseSensitivity");
    caseSensitivity.setDisplayStyle(SelectionAspect::DisplayStyle::ComboBox);
    caseSensitivity.addOption(Tr::tr("Full"));
    caseSensitivity.addOption(Tr::tr("None", "Case-sensitivity: None"));
    caseSensitivity.addOption(Tr::tr("First Letter"));
    caseSensitivity.setDefaultValue(CaseInsensitive);
    caseSensitivity.setLabelText(Tr::tr("&Case-sensitivity:"));

    completionTrigger.setSettingsKey("CompletionTrigger");
    completionTrigger.setDisplayStyle(SelectionAspect::DisplayStyle::ComboBox);
    completionTrigger.addOption(Tr::tr("Manually"));
    completionTrigger.addOption(Tr::tr("When Triggered"));
    completionTrigger.addOption(Tr::tr("Always"));
    completionTrigger.setDefaultValue(AutomaticCompletion);
    completionTrigger.setLabelText(Tr::tr("Activate completion:"));

    automaticProposalTimeoutInMs.setSettingsKey("AutomaticProposalTimeout");
    automaticProposalTimeoutInMs.setRange(0, 2000);
    automaticProposalTimeoutInMs.setSingleStep(50);
    automaticProposalTimeoutInMs.setDefaultValue(400);
    automaticProposalTimeoutInMs.setLabelText(Tr::tr("Timeout in ms:"));

    characterThreshold.setSettingsKey("CharacterThreshold");
    characterThreshold.setRange(1, 20);
    characterThreshold.setDefaultValue(3);
    characterThreshold.setLabelText(Tr::tr("Character threshold:"));

    autoInsertBrackets.setSettingsKey("AutoInsertBraces");
    autoInsertBrackets.setDefaultValue(true);
    autoInsertBrackets.setLabelText(Tr::tr("Insert opening or closing brackets"));

    surroundingAutoBrackets.setSettingsKey("SurroundingAutoBrackets");
    surroundingAutoBrackets.setDefaultValue(true);
    surroundingAutoBrackets.setLabelText(Tr::tr("Surround text selection with brackets"));
    surroundingAutoBrackets.setToolTip(
        Tr::tr("When typing a matching bracket and there is a text selection, instead of "
           "removing the selection, surrounds it with the corresponding characters."));

    autoInsertQuotes.setSettingsKey("AutoInsertQuotes");
    autoInsertQuotes.setDefaultValue(true);
    autoInsertQuotes.setLabelText(Tr::tr("Insert closing quote"));

    surroundingAutoQuotes.setSettingsKey("SurroundingAutoQuotes");
    surroundingAutoQuotes.setDefaultValue(true);
    surroundingAutoQuotes.setLabelText(Tr::tr("Surround text selection with quotes"));
    surroundingAutoQuotes.setToolTip(
        Tr::tr("When typing a matching quote and there is a text selection, instead of "
           "removing the selection, surrounds it with the corresponding characters."));

    partiallyComplete.setSettingsKey("PartiallyComplete");
    partiallyComplete.setDefaultValue(true);
    partiallyComplete.setLabelText(Tr::tr("Autocomplete common &prefix"));
    partiallyComplete.setLabelPlacement(BoolAspect::LabelPlacement::Compact);
    partiallyComplete.setToolTip(Tr::tr("Inserts the common prefix of available completion items."));

    spaceAfterFunctionName.setSettingsKey("SpaceAfterFunctionName");
    spaceAfterFunctionName.setDefaultValue(false);
    spaceAfterFunctionName.setLabelText(Tr::tr("Insert &space after function name"));

    autoSplitStrings.setSettingsKey("AutoSplitStrings");
    autoSplitStrings.setDefaultValue(true);
    autoSplitStrings.setLabelText(Tr::tr("Automatically split strings"));
    autoSplitStrings.setLabelPlacement(BoolAspect::LabelPlacement::Compact);
    autoSplitStrings.setToolTip(
        Tr::tr("Splits a string into two lines by adding an end quote at the cursor position "
           "when you press Enter and a start quote to the next line, before the rest "
           "of the string.\n\n"
           "In addition, Shift+Enter inserts an escape character at the cursor position "
           "and moves the rest of the string to the next line."));

    animateAutoComplete.setSettingsKey("AnimateAutoComplete");
    animateAutoComplete.setDefaultValue(true);
    animateAutoComplete.setLabelText(Tr::tr("Animate automatically inserted text"));
    animateAutoComplete.setToolTip(Tr::tr("Show a visual hint when for example a brace or a quote "
                                       "is automatically inserted by the editor."));

    highlightAutoComplete.setSettingsKey("HighlightAutoComplete");
    highlightAutoComplete.setDefaultValue(true);
    highlightAutoComplete.setLabelText(Tr::tr("Highlight automatically inserted text"));

    skipAutoCompletedText.setSettingsKey("SkipAutoComplete");
    skipAutoCompletedText.setDefaultValue(true);
    skipAutoCompletedText.setLabelText(Tr::tr("Skip automatically inserted character when typing"));
    skipAutoCompletedText.setToolTip(Tr::tr("Skip automatically inserted character if re-typed manually "
                                            "after completion or by pressing tab."));

    autoRemove.setSettingsKey("AutoRemove");
    autoRemove.setDefaultValue(true);
    autoRemove.setLabelText(Tr::tr("Remove automatically inserted text on backspace"));
    autoRemove.setToolTip(Tr::tr("Remove the automatically inserted character if the trigger "
                                 "is deleted by backspace after the completion."));

    overwriteClosingChars.setSettingsKey("OverwriteClosingChars");
    overwriteClosingChars.setDefaultValue(false);
    overwriteClosingChars.setLabelText(Tr::tr("Overwrite closing punctuation"));
    overwriteClosingChars.setToolTip(Tr::tr("Automatically overwrite closing parentheses and quotes."));

    setLayouter([this] {
        using namespace Layouting;
        return Column {
            Group {
                title(Tr::tr("Behavior")),
                Form {
                    caseSensitivity, st, br,
                    completionTrigger, st, br,
                    automaticProposalTimeoutInMs, st, br,
                    characterThreshold, st, br,
                    Span(2, partiallyComplete), br,
                    Span(2, autoSplitStrings), br,
                }
            },
            Group {
                title(Tr::tr("&Automatically Insert Matching Characters")),
                Row {
                    Column {
                        autoInsertBrackets,
                        surroundingAutoBrackets,
                        spaceAfterFunctionName,
                        highlightAutoComplete,
                        Row { Space(30), skipAutoCompletedText },
                        Row { Space(30), autoRemove },
                    },
                    Column {
                        autoInsertQuotes,
                        surroundingAutoQuotes,
                        animateAutoComplete,
                        overwriteClosingChars,
                        st,
                    }
                }
            },
            st
        };
    });

    readSettings();

    skipAutoCompletedText.setEnabler(&highlightAutoComplete);
    autoRemove.setEnabler(&highlightAutoComplete);

    auto updateTimeout = [this] {
        automaticProposalTimeoutInMs.setEnabled(completionTrigger.volatileValue() == AutomaticCompletion);
    };

    completionTrigger.addOnVolatileValueChanged(this, updateTimeout);
    updateTimeout();
}

namespace Internal {

class CompletionSettingsPage final : public Core::IOptionsPage
{
public:
    CompletionSettingsPage()
    {
        setId("P.Completion");
        setDisplayName(Tr::tr("Completion"));
        setCategory(TextEditor::Constants::TEXT_EDITOR_SETTINGS_CATEGORY);
        setSettingsProvider([] { return &globalCompletionSettings(); });
    }
};

void setupCompletionSettings()
{
    static CompletionSettingsPage theCompletionSettingsPage;
}

} // Internal
} // TextEditor
