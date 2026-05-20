// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "commentssettings.h"

#include "texteditorconstants.h"
#include "texteditortr.h"

#include <coreplugin/icore.h>
#include <utils/layoutbuilder.h>

using namespace Utils;

namespace TextEditor {

CommentsSettings &CommentsSettings::instance()
{
    static CommentsSettings settings;
    return settings;
}

namespace {
const char kDocumentationCommentsGroup[] = "CppToolsDocumentationComments";
const char kEnableDoxygenBlocks[] = "EnableDoxygenBlocks";
const char kGenerateBrief[] = "GenerateBrief";
const char kAddLeadingAsterisks[] = "AddLeadingAsterisks";
const char kCommandPrefix[] = "CommandPrefix";
}

Key CommentsSettings::mainSettingsKey() { return kDocumentationCommentsGroup; }

CommentsSettings::CommentsSettings()
{
    setAutoApply(false);
    setSettingsGroup(kDocumentationCommentsGroup);

    enableDoxygen.setSettingsKey(kEnableDoxygenBlocks);
    enableDoxygen.setDefaultValue(true);
    enableDoxygen.setLabelText(Tr::tr("Enable Doxygen blocks"));
    enableDoxygen.setToolTip(
        Tr::tr("Automatically creates a Doxygen comment upon pressing "
               "enter after a '/**', '/*!', '//!' or '///'."));

    generateBrief.setSettingsKey(kGenerateBrief);
    generateBrief.setDefaultValue(true);
    generateBrief.setLabelText(Tr::tr("Generate brief description"));
    generateBrief.setToolTip(
        Tr::tr("Generates a <i>brief</i> command with an initial "
               "description for the corresponding declaration."));

    leadingAsterisks.setSettingsKey(kAddLeadingAsterisks);
    leadingAsterisks.setDefaultValue(true);
    leadingAsterisks.setLabelText(Tr::tr("Add leading asterisks"));
    leadingAsterisks.setToolTip(
        Tr::tr("Adds leading asterisks when continuing C/C++ \"/*\", Qt \"/*!\" "
               "and Java \"/**\" style comments on new lines."));

    commandPrefix.setSettingsKey(kCommandPrefix);
    commandPrefix.setDisplayStyle(SelectionAspect::DisplayStyle::ComboBox);
    commandPrefix.addOption(Tr::tr("Automatic"));
    commandPrefix.addOption("@");
    commandPrefix.addOption("\\");
    commandPrefix.setDefaultValue(CommandPrefix::Auto);
    commandPrefix.setLabelText(Tr::tr("Doxygen command prefix:"));
    commandPrefix.setToolTip(Tr::tr(R"(Doxygen allows "@" and "\" to start commands.
        By default, "@" is used if the surrounding comment starts with "/**" or "///", and "\" is used
        if the comment starts with "/*!" or "//!".)"
    ));

    setLayouter([this] {
        using namespace Layouting;
        return Column {
            enableDoxygen,
            Row { Space(30), generateBrief },
            leadingAsterisks,
            Row { commandPrefix, st },
            st
        };
    });

    readSettings();

    generateBrief.setEnabler(&enableDoxygen);
}

CommentsSettings::Data CommentsSettings::data() const
{
    Data d;
    d.commandPrefix = commandPrefix();
    d.enableDoxygen = enableDoxygen();
    d.generateBrief = generateBrief();
    d.leadingAsterisks = leadingAsterisks();
    return d;
}

void CommentsSettings::setData(const Data &data)
{
    commandPrefix.setValue(data.commandPrefix);
    enableDoxygen.setValue(data.enableDoxygen);
    generateBrief.setValue(data.generateBrief);
    leadingAsterisks.setValue(data.leadingAsterisks);

    writeSettings();
}

void CommentsSettings::Data::fromMap(const Store &data)
{
    enableDoxygen = data.value(kEnableDoxygenBlocks, enableDoxygen).toBool();
    generateBrief = data.value(kGenerateBrief, generateBrief).toBool();
    leadingAsterisks = data.value(kAddLeadingAsterisks, leadingAsterisks).toBool();
    commandPrefix = static_cast<CommentsSettings::CommandPrefix>(
            data.value(kCommandPrefix, int(commandPrefix)).toInt());
}

bool operator==(const CommentsSettings::Data &a, const CommentsSettings::Data &b)
{
    return a.enableDoxygen == b.enableDoxygen
           && a.commandPrefix == b.commandPrefix
           && a.generateBrief == b.generateBrief
           && a.leadingAsterisks == b.leadingAsterisks;
}

void CommentsSettings::Data::toMap(Store &data) const
{
    data.insert(kEnableDoxygenBlocks, enableDoxygen);
    data.insert(kGenerateBrief, generateBrief);
    data.insert(kAddLeadingAsterisks, leadingAsterisks);
    data.insert(kCommandPrefix, int(commandPrefix));
}

namespace Internal {

class CommentsSettingsPage final : public Core::IOptionsPage
{
public:
    CommentsSettingsPage()
    {
        setId(Constants::TEXT_EDITOR_COMMENTS_SETTINGS);
        setDisplayName(Tr::tr("Documentation Comments"));
        setCategory(TextEditor::Constants::TEXT_EDITOR_SETTINGS_CATEGORY);
        setSettingsProvider([] { return &CommentsSettings::instance(); });
    }
};

void setupCommentsSettings()
{
    static CommentsSettingsPage theCommentsSettingsPage;
}

} // namespace Internal
} // namespace TextEditor
