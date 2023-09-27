// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "commentssettings.h"

#include "texteditorconstants.h"
#include "texteditorsettings.h"
#include "texteditortr.h"

#include <coreplugin/icore.h>
#include <projectexplorer/project.h>
#include <utils/layoutbuilder.h>

#include <QCheckBox>
#include <QComboBox>

using namespace Layouting;
using namespace Utils;

namespace TextEditor {

namespace {
const char kDocumentationCommentsGroup[] = "CppToolsDocumentationComments";
const char kEnableDoxygenBlocks[] = "EnableDoxygenBlocks";
const char kGenerateBrief[] = "GenerateBrief";
const char kAddLeadingAsterisks[] = "AddLeadingAsterisks";
const char kCommandPrefix[] = "CommandPrefix";
}

void CommentsSettings::setData(const Data &data)
{
    if (data == instance().m_data)
        return;
    instance().m_data = data;
    instance().save();
}

Key CommentsSettings::mainSettingsKey() { return kDocumentationCommentsGroup; }
Key CommentsSettings::enableDoxygenSettingsKey() { return kEnableDoxygenBlocks; }
Key CommentsSettings::generateBriefSettingsKey() { return kGenerateBrief; }
Key CommentsSettings::leadingAsterisksSettingsKey() { return kAddLeadingAsterisks; }
Key CommentsSettings::commandPrefixKey() { return kCommandPrefix; }

CommentsSettings::CommentsSettings()
{
    load();
}

CommentsSettings &CommentsSettings::instance()
{
    static CommentsSettings settings;
    return settings;
}

void CommentsSettings::save() const
{
    Utils::QtcSettings * const s = Core::ICore::settings();
    s->beginGroup(kDocumentationCommentsGroup);
    s->setValue(kEnableDoxygenBlocks, m_data.enableDoxygen);
    s->setValue(kGenerateBrief, m_data.generateBrief);
    s->setValue(kAddLeadingAsterisks, m_data.leadingAsterisks);
    s->setValueWithDefault(kCommandPrefix, int(m_data.commandPrefix));
    s->endGroup();
}

void CommentsSettings::load()
{
    Utils::QtcSettings * const s = Core::ICore::settings();
    s->beginGroup(kDocumentationCommentsGroup);
    m_data.enableDoxygen = s->value(kEnableDoxygenBlocks, true).toBool();
    m_data.generateBrief = m_data.enableDoxygen && s->value(kGenerateBrief, true).toBool();
    m_data.leadingAsterisks = s->value(kAddLeadingAsterisks, true).toBool();
    m_data.commandPrefix = static_cast<CommandPrefix>(
        s->value(kCommandPrefix, int(m_data.commandPrefix)).toInt());
    s->endGroup();
}

class CommentsSettingsWidget::Private
{
public:
    QCheckBox m_overwriteClosingChars;
    QCheckBox m_enableDoxygenCheckBox;
    QCheckBox m_generateBriefCheckBox;
    QCheckBox m_leadingAsterisksCheckBox;
    QComboBox m_commandPrefixComboBox;
};

CommentsSettingsWidget::CommentsSettingsWidget(const CommentsSettings::Data &settings)
    : d(new Private)
{
    d->m_enableDoxygenCheckBox.setText(Tr::tr("Enable Doxygen blocks"));
    d->m_enableDoxygenCheckBox.setToolTip(
        Tr::tr("Automatically creates a Doxygen comment upon pressing "
               "enter after a '/**', '/*!', '//!' or '///'."));

    d->m_generateBriefCheckBox.setText(Tr::tr("Generate brief description"));
    d->m_generateBriefCheckBox.setToolTip(
        Tr::tr("Generates a <i>brief</i> command with an initial "
               "description for the corresponding declaration."));

    d->m_leadingAsterisksCheckBox.setText(Tr::tr("Add leading asterisks"));
    d->m_leadingAsterisksCheckBox.setToolTip(
        Tr::tr("Adds leading asterisks when continuing C/C++ \"/*\", Qt \"/*!\" "
               "and Java \"/**\" style comments on new lines."));

    const auto commandPrefixLabel = new QLabel(Tr::tr("Doxygen command prefix:"));
    const QString commandPrefixToolTip = Tr::tr(R"(Doxygen allows "@" and "\" to start commands.
By default, "@" is used if the surrounding comment starts with "/**" or "///", and "\" is used
if the comment starts with "/*!" or "//!)");
    commandPrefixLabel->setToolTip(commandPrefixToolTip);
    d->m_commandPrefixComboBox.setToolTip(commandPrefixToolTip);
    d->m_commandPrefixComboBox.addItem(Tr::tr("Automatic"));
    d->m_commandPrefixComboBox.addItem("@");
    d->m_commandPrefixComboBox.addItem("\\");

    initFromSettings(settings);

    Column {
        &d->m_enableDoxygenCheckBox,
        Row { Space(30), &d->m_generateBriefCheckBox },
        &d->m_leadingAsterisksCheckBox,
        Row { commandPrefixLabel, &d->m_commandPrefixComboBox, st },
        st
    }.attachTo(this);

    connect(&d->m_enableDoxygenCheckBox, &QCheckBox::toggled,
            &d->m_generateBriefCheckBox, &QCheckBox::setEnabled);

    for (QCheckBox *checkBox : {&d->m_enableDoxygenCheckBox, &d->m_generateBriefCheckBox,
                                &d->m_leadingAsterisksCheckBox}) {
        connect(checkBox, &QCheckBox::clicked, this, &CommentsSettingsWidget::settingsChanged);
    }
    connect(&d->m_commandPrefixComboBox, &QComboBox::currentIndexChanged,
            this, &CommentsSettingsWidget::settingsChanged);
}

CommentsSettingsWidget::~CommentsSettingsWidget() { delete d; }

CommentsSettings::Data CommentsSettingsWidget::settingsData() const
{
    CommentsSettings::Data settings;
    settings.enableDoxygen = d->m_enableDoxygenCheckBox.isChecked();
    settings.generateBrief = d->m_generateBriefCheckBox.isChecked();
    settings.leadingAsterisks = d->m_leadingAsterisksCheckBox.isChecked();
    settings.commandPrefix = static_cast<CommentsSettings::CommandPrefix>(
        d->m_commandPrefixComboBox.currentIndex());
    return settings;
}

void CommentsSettingsWidget::apply()
{
    // This function is only ever called for the global settings page.
    CommentsSettings::setData(settingsData());
}

void CommentsSettingsWidget::initFromSettings(const CommentsSettings::Data &settings)
{
    d->m_enableDoxygenCheckBox.setChecked(settings.enableDoxygen);
    d->m_generateBriefCheckBox.setEnabled(d->m_enableDoxygenCheckBox.isChecked());
    d->m_generateBriefCheckBox.setChecked(settings.generateBrief);
    d->m_leadingAsterisksCheckBox.setChecked(settings.leadingAsterisks);
    d->m_commandPrefixComboBox.setCurrentIndex(int(settings.commandPrefix));
}

namespace Internal {

CommentsSettingsPage::CommentsSettingsPage()
{
    setId(Constants::TEXT_EDITOR_COMMENTS_SETTINGS);
    setDisplayName(Tr::tr("Documentation Comments"));
    setCategory(TextEditor::Constants::TEXT_EDITOR_SETTINGS_CATEGORY);
    setDisplayCategory(Tr::tr("Text Editor"));
    setCategoryIconPath(TextEditor::Constants::TEXT_EDITOR_SETTINGS_CATEGORY_ICON_PATH);
    setWidgetCreator([] { return new CommentsSettingsWidget(CommentsSettings::data()); });
}

} // namespace Internal
} // namespace TextEditor
