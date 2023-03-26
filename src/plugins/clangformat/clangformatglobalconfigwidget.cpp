// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "clangformatglobalconfigwidget.h"

#include "clangformatconstants.h"
#include "clangformatsettings.h"
#include "clangformattr.h"
#include "clangformatutils.h"

#include <projectexplorer/project.h>

#include <utils/layoutbuilder.h>

#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QWidget>

#include <sstream>

using namespace ProjectExplorer;
using namespace Utils;

namespace ClangFormat {

ClangFormatGlobalConfigWidget::ClangFormatGlobalConfigWidget(ProjectExplorer::Project *project,
                                                             QWidget *parent)
    : CppCodeStyleWidget(parent)
    , m_project(project)
{
    resize(489, 305);

    m_projectHasClangFormat = new QLabel(this);
    m_formattingModeLabel = new QLabel(Tr::tr("Formatting mode:"));
    m_indentingOrFormatting = new QComboBox(this);
    m_formatWhileTyping = new QCheckBox(Tr::tr("Format while typing"));
    m_formatOnSave = new QCheckBox(Tr::tr("Format edited code on file save"));
    m_overrideDefault = new QCheckBox(Tr::tr("Override Clang Format configuration file"));
    m_useGlobalSettings = new QCheckBox(Tr::tr("Use global settings"));
    m_useGlobalSettings->hide();

    using namespace Layouting;

    Group globalSettingsGroupBox {
        title(Tr::tr("ClangFormat settings:")),
            Column {
                m_useGlobalSettings,
                Row { m_formattingModeLabel, m_indentingOrFormatting, st },
                m_formatWhileTyping,
                m_formatOnSave,
                m_projectHasClangFormat,
                m_overrideDefault
        }
    };

    Column {
        globalSettingsGroupBox
    }.attachTo(this, Layouting::WithoutMargins);

    initCheckBoxes();
    initIndentationOrFormattingCombobox();
    initOverrideCheckBox();
    initUseGlobalSettingsCheckBox();

    if (project) {
        m_formattingModeLabel->hide();
        m_formatOnSave->hide();
        m_formatWhileTyping->hide();

        m_useGlobalSettings->show();
        return;
    }
    globalSettingsGroupBox.widget->show();
}

ClangFormatGlobalConfigWidget::~ClangFormatGlobalConfigWidget() = default;

void ClangFormatGlobalConfigWidget::initCheckBoxes()
{
    auto setEnableCheckBoxes = [this](int index) {
        bool isFormatting = index == static_cast<int>(ClangFormatSettings::Mode::Formatting);

        m_formatOnSave->setEnabled(isFormatting);
        m_formatWhileTyping->setEnabled(isFormatting);
    };
    setEnableCheckBoxes(m_indentingOrFormatting->currentIndex());
    connect(m_indentingOrFormatting, &QComboBox::currentIndexChanged,
            this, setEnableCheckBoxes);

    m_formatOnSave->setChecked(ClangFormatSettings::instance().formatOnSave());
    m_formatWhileTyping->setChecked(ClangFormatSettings::instance().formatWhileTyping());
}

void ClangFormatGlobalConfigWidget::initIndentationOrFormattingCombobox()
{
    m_indentingOrFormatting->insertItem(static_cast<int>(ClangFormatSettings::Mode::Indenting),
                                        Tr::tr("Indenting only"));
    m_indentingOrFormatting->insertItem(static_cast<int>(ClangFormatSettings::Mode::Formatting),
                                        Tr::tr("Full formatting"));
    m_indentingOrFormatting->insertItem(static_cast<int>(ClangFormatSettings::Mode::Disable),
                                        Tr::tr("Disable"));

    m_indentingOrFormatting->setCurrentIndex(
        static_cast<int>(getProjectIndentationOrFormattingSettings(m_project)));

    connect(m_indentingOrFormatting, &QComboBox::currentIndexChanged, this, [this](int index) {
        if (m_project)
            m_project->setNamedSettings(Constants::MODE_ID, index);
    });
}

void ClangFormatGlobalConfigWidget::initUseGlobalSettingsCheckBox()
{
    if (!m_project)
        return;

    const auto enableProjectSettings = [this] {
        const bool isDisabled = m_project && m_useGlobalSettings->isChecked();
        m_indentingOrFormatting->setDisabled(isDisabled);
        m_overrideDefault->setDisabled(isDisabled
                                       || (m_indentingOrFormatting->currentIndex()
                                           == static_cast<int>(ClangFormatSettings::Mode::Disable)));
    };

    m_useGlobalSettings->setChecked(getProjectUseGlobalSettings(m_project));
    enableProjectSettings();

    connect(m_useGlobalSettings, &QCheckBox::toggled,
            this, [this, enableProjectSettings] (bool checked) {
                m_project->setNamedSettings(Constants::USE_GLOBAL_SETTINGS, checked);
                enableProjectSettings();
            });
}

bool ClangFormatGlobalConfigWidget::projectClangFormatFileExists()
{
    llvm::Expected<clang::format::FormatStyle> styleFromProjectFolder
        = clang::format::getStyle("file", m_project->projectFilePath().path().toStdString(), "none");

    return styleFromProjectFolder && !(*styleFromProjectFolder == clang::format::getNoStyle());
}

void ClangFormatGlobalConfigWidget::initOverrideCheckBox()
{
    if (!m_project || !projectClangFormatFileExists()) {
        m_projectHasClangFormat->hide();
    } else {
        m_projectHasClangFormat->show();
        m_projectHasClangFormat->setText(Tr::tr("The current project has its own .clang-format file which "
                                                "can be overridden by the settings below."));
    }

    auto setEnableOverrideCheckBox = [this](int index) {
        bool isDisable = index == static_cast<int>(ClangFormatSettings::Mode::Disable);
        m_overrideDefault->setDisabled(isDisable);
    };

    setEnableOverrideCheckBox(m_indentingOrFormatting->currentIndex());
    connect(m_indentingOrFormatting, &QComboBox::currentIndexChanged,
            this, setEnableOverrideCheckBox);

    m_overrideDefault->setToolTip(
        Tr::tr("Override Clang Format configuration file with the chosen configuration."));

    m_overrideDefault->setChecked(getProjectOverriddenSettings(m_project));

    connect(m_overrideDefault, &QCheckBox::toggled, this, [this](bool checked) {
        if (m_project)
            m_project->setNamedSettings(Constants::OVERRIDE_FILE_ID, checked);
    });
}


void ClangFormatGlobalConfigWidget::apply()
{
    ClangFormatSettings &settings = ClangFormatSettings::instance();
    settings.setFormatOnSave(m_formatOnSave->isChecked());
    settings.setFormatWhileTyping(m_formatWhileTyping->isChecked());
    if (!m_project) {
        settings.setMode(
            static_cast<ClangFormatSettings::Mode>(m_indentingOrFormatting->currentIndex()));
        settings.setOverrideDefaultFile(m_overrideDefault->isChecked());
    }
    settings.write();
}

} // namespace ClangFormat
