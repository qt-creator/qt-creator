// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "commentssettings.h"

#include "texteditorconstants.h"
#include "texteditorsettings.h"
#include "texteditortr.h"

#include <coreplugin/icore.h>
#include <utils/layoutbuilder.h>

#include <QCheckBox>
#include <QSettings>

using namespace Layouting;

namespace TextEditor {

namespace {
const char kDocumentationCommentsGroup[] = "CppToolsDocumentationComments";
const char kEnableDoxygenBlocks[] = "EnableDoxygenBlocks";
const char kGenerateBrief[] = "GenerateBrief";
const char kAddLeadingAsterisks[] = "AddLeadingAsterisks";
}

bool operator==(const CommentsSettings::Data &a, const CommentsSettings::Data &b)
{
    return a.enableDoxygen == b.enableDoxygen
           && a.generateBrief == b.generateBrief
           && a.leadingAsterisks == b.leadingAsterisks;
}

void CommentsSettings::setData(const Data &data)
{
    if (data == instance().m_data)
        return;
    instance().m_data = data;
    instance().save();
    emit TextEditorSettings::instance()->commentsSettingsChanged(data);
}

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
    s->endGroup();
}

void CommentsSettings::load()
{
    Utils::QtcSettings * const s = Core::ICore::settings();
    s->beginGroup(kDocumentationCommentsGroup);
    m_data.enableDoxygen = s->value(kEnableDoxygenBlocks, true).toBool();
    m_data.generateBrief = m_data.enableDoxygen && s->value(kGenerateBrief, true).toBool();
    m_data.leadingAsterisks = s->value(kAddLeadingAsterisks, true).toBool();
    s->endGroup();
}

namespace Internal {

class CommentsSettingsWidget final : public Core::IOptionsPageWidget
{
public:
    explicit CommentsSettingsWidget()
    {
        initFromSettings();

        m_enableDoxygenCheckBox.setText(Tr::tr("Enable Doxygen blocks"));
        m_enableDoxygenCheckBox.setToolTip(
            Tr::tr("Automatically creates a Doxygen comment upon pressing "
                   "enter after a '/**', '/*!', '//!' or '///'."));

        m_generateBriefCheckBox.setText(Tr::tr("Generate brief description"));
        m_generateBriefCheckBox.setToolTip(
            Tr::tr("Generates a <i>brief</i> command with an initial "
                   "description for the corresponding declaration."));

        m_leadingAsterisksCheckBox.setText(Tr::tr("Add leading asterisks"));
        m_leadingAsterisksCheckBox.setToolTip(
            Tr::tr("Adds leading asterisks when continuing C/C++ \"/*\", Qt \"/*!\" "
                   "and Java \"/**\" style comments on new lines."));

        Column {
            &m_enableDoxygenCheckBox,
            Row { Space(30), &m_generateBriefCheckBox },
            &m_leadingAsterisksCheckBox,
            st
        }.attachTo(this);

        connect(&m_enableDoxygenCheckBox, &QCheckBox::toggled,
                &m_generateBriefCheckBox, &QCheckBox::setEnabled);
    }

private:
    void initFromSettings()
    {
        const CommentsSettings::Data &settings = CommentsSettings::data();
        m_enableDoxygenCheckBox.setChecked(settings.enableDoxygen);
        m_generateBriefCheckBox.setEnabled(m_enableDoxygenCheckBox.isChecked());
        m_generateBriefCheckBox.setChecked(settings.generateBrief);
        m_leadingAsterisksCheckBox.setChecked(settings.leadingAsterisks);
    }

    CommentsSettings::Data toSettings() const
    {
        CommentsSettings::Data settings;
        settings.enableDoxygen = m_enableDoxygenCheckBox.isChecked();
        settings.generateBrief = m_generateBriefCheckBox.isChecked();
        settings.leadingAsterisks = m_leadingAsterisksCheckBox.isChecked();
        return settings;
    }

    void apply() override
    {
        CommentsSettings::setData(toSettings());
    }

    QCheckBox m_overwriteClosingChars;
    QCheckBox m_enableDoxygenCheckBox;
    QCheckBox m_generateBriefCheckBox;
    QCheckBox m_leadingAsterisksCheckBox;
};

CommentsSettingsPage::CommentsSettingsPage()
{
    setId("Q.Comments");
    setDisplayName(Tr::tr("Documentation Comments"));
    setCategory(TextEditor::Constants::TEXT_EDITOR_SETTINGS_CATEGORY);
    setDisplayCategory(Tr::tr("Text Editor"));
    setCategoryIconPath(TextEditor::Constants::TEXT_EDITOR_SETTINGS_CATEGORY_ICON_PATH);
    setWidgetCreator([] { return new CommentsSettingsWidget; });
}

} // namespace Internal
} // namespace TextEditor
