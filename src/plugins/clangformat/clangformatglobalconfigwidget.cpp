// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "clangformatglobalconfigwidget.h"
#include "clangformatconstants.h"
#include "clangformatfile.h"
#include "clangformatsettings.h"
#include "clangformattr.h"
#include "clangformatutils.h"

#include <cppeditor/cppcodestylesettingspage.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projecttree.h>
#include <texteditor/codestylepool.h>
#include <texteditor/codestyleselectorwidget.h>
#include <utils/fileutils.h>
#include <utils/layoutbuilder.h>

#include <QCheckBox>
#include <QSpinBox>

using namespace CppEditor;
using namespace ProjectExplorer;
using namespace TextEditor;
using namespace Utils;

namespace ClangFormat {

ClangFormatGlobalConfigWidget::ClangFormatGlobalConfigWidget(
    Project *project, ICodeStylePreferences *codeStyle, QWidget *parent)
    : QWidget(parent)
    , m_project(project)
    , m_codeStyle(codeStyle)
{
    const QString sizeThresholdToolTip = Tr::tr(
        "Files greater than this will not be indented by ClangFormat.\n"
        "The built-in code indenter will handle indentation.");

    m_projectHasClangFormat = new QLabel(this);
    m_formattingModeLabel = new QLabel(Tr::tr("Formatting mode:"));
    m_fileSizeThresholdLabel = new QLabel(Tr::tr("Ignore files greater than:"));
    m_fileSizeThresholdLabel->setToolTip(sizeThresholdToolTip);
    m_fileSizeThresholdSpinBox = new QSpinBox(this);
    m_fileSizeThresholdSpinBox->setToolTip(sizeThresholdToolTip);
    m_indentingOrFormatting = new QComboBox(this);
    m_formatWhileTyping = new QCheckBox(Tr::tr("Format while typing"));
    m_formatOnSave = new QCheckBox(Tr::tr("Format edited code on file save"));
    m_useCustomSettingsCheckBox = new QCheckBox(Tr::tr("Use custom settings"));
    m_useClangFormat = new QCheckBox(Tr::tr("Use ClangFormat"));
    m_useGlobalSettings = new QCheckBox(Tr::tr("Use global settings"));
    m_useGlobalSettings->hide();
    m_useCustomSettings = clangFormatSettings().useCustomSettings();

    m_currentProjectLabel = new Utils::InfoLabel(
        Tr::tr("Please note that the current project includes a .clang-format file, which will be "
               "used for code indenting and formatting."),
        Utils::InfoLabel::Warning);
    m_currentProjectLabel->setWordWrap(true);

    using namespace Layouting;

    m_clangFormatOptions = new QWidget;

    // clang-format off
    // Only the two checkboxes gated by the formatting mode are indented, to show
    // that the mode above enables them; everything else stays at the base level.
    Column {
        Form {
             m_fileSizeThresholdLabel, m_fileSizeThresholdSpinBox, st, br,
             m_formattingModeLabel, m_indentingOrFormatting, st, br
        },
        Column {
            m_formatWhileTyping,
            m_formatOnSave,
            customMargins(20, 0, 0, 0)
        },
        m_useCustomSettingsCheckBox,
        m_projectHasClangFormat,
        m_currentProjectLabel,
        noMargin
    }.attachTo(m_clangFormatOptions);

    Column {
        m_useGlobalSettings,
        m_useClangFormat,
        m_clangFormatOptions,
        noMargin
    }.attachTo(this);
    // clang-format on

    initCheckBoxes();
    initIndentationOrFormattingCombobox();
    initCustomSettingsCheckBox();
    initUseGlobalSettingsCheckBox();
    initFileSizeThresholdSpinBox();
    initCurrentProjectLabel();

    if (project) {
        m_formatOnSave->hide();
        m_formatWhileTyping->hide();
        m_useGlobalSettings->show();
        return;
    }

    installMarkSettingsDirtyTrigger(m_useClangFormat);
    installMarkSettingsDirtyTrigger(m_indentingOrFormatting);
    installMarkSettingsDirtyTrigger(m_fileSizeThresholdSpinBox);
    installMarkSettingsDirtyTrigger(m_formatWhileTyping);
    installMarkSettingsDirtyTrigger(m_formatOnSave);
    installMarkSettingsDirtyTrigger(m_useCustomSettingsCheckBox);
}

void ClangFormatGlobalConfigWidget::initCheckBoxes()
{
    auto setEnableCheckBoxes = [this](int index) {
        bool isFormatting = index == static_cast<int>(ClangFormatSettings::Mode::Formatting);
        m_formatOnSave->setEnabled(isFormatting);
        m_formatWhileTyping->setEnabled(isFormatting);
    };
    setEnableCheckBoxes(m_indentingOrFormatting->currentIndex());
    connect(m_indentingOrFormatting, &QComboBox::currentIndexChanged, this, setEnableCheckBoxes);

    m_formatOnSave->setChecked(clangFormatSettings().formatOnSave());
    m_formatWhileTyping->setChecked(clangFormatSettings().formatWhileTyping());
}

void ClangFormatGlobalConfigWidget::initIndentationOrFormattingCombobox()
{
    m_indentingOrFormatting->insertItem(
        static_cast<int>(ClangFormatSettings::Mode::Indenting), Tr::tr("Indenting only"));
    m_indentingOrFormatting->insertItem(
        static_cast<int>(ClangFormatSettings::Mode::Formatting), Tr::tr("Full formatting"));

    const ClangFormatSettings::Mode currentMode =
        getProjectIndentationOrFormattingSettings(m_project);
    const bool clangFormatEnabled = (currentMode != ClangFormatSettings::Mode::Disable);
    m_useClangFormat->setChecked(clangFormatEnabled);
    m_clangFormatOptions->setVisible(clangFormatEnabled);
    if (clangFormatEnabled)
        m_indentingOrFormatting->setCurrentIndex(static_cast<int>(currentMode));

    connect(m_useClangFormat, &QCheckBox::toggled, this, [this](bool checked) {
        m_clangFormatOptions->setVisible(checked);
        const ClangFormatSettings::Mode newMode = checked
            ? static_cast<ClangFormatSettings::Mode>(m_indentingOrFormatting->currentIndex())
            : ClangFormatSettings::Mode::Disable;
        if (m_project)
            m_project->setNamedSettings(Constants::MODE_ID, static_cast<int>(newMode));
        emit modeChanged(newMode);
    });

    connect(m_indentingOrFormatting, &QComboBox::currentIndexChanged, this, [this](int index) {
        if (m_project)
            m_project->setNamedSettings(Constants::MODE_ID, index);
        emit modeChanged(static_cast<ClangFormatSettings::Mode>(index));
    });
}

void ClangFormatGlobalConfigWidget::initUseGlobalSettingsCheckBox()
{
    if (!m_project)
        return;

    const auto enableProjectSettings = [this] {
        const bool useGlobal = m_useGlobalSettings->isChecked();
        m_useClangFormat->setEnabled(!useGlobal);
        m_clangFormatOptions->setEnabled(!useGlobal);
        m_useCustomSettingsCheckBox->setChecked(getProjectCustomSettings(m_project));
        emit m_codeStyle->currentPreferencesChanged(m_codeStyle->currentPreferences());
    };

    m_useGlobalSettings->setChecked(getProjectUseGlobalSettings(m_project));
    enableProjectSettings();

    connect(
        m_useGlobalSettings, &QCheckBox::toggled, this, [this, enableProjectSettings](bool checked) {
            m_project->setNamedSettings(Constants::USE_GLOBAL_SETTINGS, checked);
            enableProjectSettings();
            emit modeChanged(mode());
        });
}

void ClangFormatGlobalConfigWidget::initFileSizeThresholdSpinBox()
{
    m_fileSizeThresholdSpinBox->setMinimum(1);
    m_fileSizeThresholdSpinBox->setMaximum(std::numeric_limits<int>::max());
    m_fileSizeThresholdSpinBox->setSuffix(" KB");
    m_fileSizeThresholdSpinBox->setValue(clangFormatSettings().fileSizeThreshold());
    if (m_project) {
        m_fileSizeThresholdSpinBox->hide();
        m_fileSizeThresholdLabel->hide();
    }
}

void ClangFormatGlobalConfigWidget::initCurrentProjectLabel()
{
    auto setCurrentProjectLabelVisible = [this]() {
        if (!m_useClangFormat->isChecked()) {
            m_currentProjectLabel->hide();
            return;
        }
        ProjectExplorer::Project *currentProject
            = m_project ? m_project : ProjectExplorer::ProjectTree::currentProject();

        if (currentProject) {
            Utils::FilePath settingsFilePath = currentProject->projectDirectory()
                                               / Constants::SETTINGS_FILE_NAME;
            Utils::FilePath settingsAltFilePath = currentProject->projectDirectory()
                                                  / Constants::SETTINGS_FILE_ALT_NAME;

            if ((settingsFilePath.exists() || settingsAltFilePath.exists())
                && m_useCustomSettingsCheckBox->checkState() == Qt::CheckState::Unchecked) {
                m_currentProjectLabel->show();
                return;
            }
        }
        m_currentProjectLabel->hide();
    };
    setCurrentProjectLabelVisible();
    connect(m_useCustomSettingsCheckBox, &QCheckBox::toggled, this, setCurrentProjectLabelVisible);
    connect(m_useGlobalSettings, &QCheckBox::toggled, this, setCurrentProjectLabelVisible);
    connect(m_useClangFormat, &QCheckBox::toggled, this, setCurrentProjectLabelVisible);
    connect(m_indentingOrFormatting,
            &QComboBox::currentIndexChanged,
            this,
            [setCurrentProjectLabelVisible](int) { setCurrentProjectLabelVisible(); });
}

bool ClangFormatGlobalConfigWidget::projectClangFormatFileExists()
{
    llvm::Expected<clang::format::FormatStyle> styleFromProjectFolder = clang::format::getStyle(
        "file", m_project->projectFilePath().path().toStdString(), "none", "", nullptr, true);

    return styleFromProjectFolder && !(*styleFromProjectFolder == clang::format::getNoStyle());
}

void ClangFormatGlobalConfigWidget::initCustomSettingsCheckBox()
{
    if (!m_project || !projectClangFormatFileExists()) {
        m_projectHasClangFormat->hide();
    } else {
        m_projectHasClangFormat->show();
        m_projectHasClangFormat->setText(
            Tr::tr("The current project has its own .clang-format file which "
                   "can be overridden by the settings below."));
    }

    m_useCustomSettingsCheckBox->setChecked(getProjectCustomSettings(m_project));
    m_useCustomSettingsCheckBox->setToolTip(
        "<html>"
        + Tr::tr("When this option is enabled, ClangFormat will use a "
                 "user-specified configuration from the widget below, "
                 "instead of the project .clang-format file. You can "
                 "customize the formatting options for your code by "
                 "adjusting the settings in the widget. Note that any "
                 "changes made there will only affect the current "
                 "configuration, and will not modify the project "
                 ".clang-format file."));

    connect(m_useCustomSettingsCheckBox, &QCheckBox::toggled, this, [this](bool checked) {
        if (m_project) {
            m_project->setNamedSettings(Constants::USE_CUSTOM_SETTINGS_ID, checked);
        } else {
            clangFormatSettings().useCustomSettings.setValue(checked);
        }
        emit useCustomSettingsChanged(checked);
    });
}

void ClangFormatGlobalConfigWidget::apply()
{
    // All settings written here are global. In the project view they are hidden
    // and only mirror the loaded global values, so writing them back would just
    // rewrite the global settings with their own contents.
    if (m_project)
        return;

    ClangFormatSettings &settings = clangFormatSettings();
    settings.formatOnSave.setValue(m_formatOnSave->isChecked());
    settings.formatWhileTyping.setValue(m_formatWhileTyping->isChecked());
    settings.mode.setValue(mode());
    settings.useCustomSettings.setValue(m_useCustomSettingsCheckBox->isChecked());
    settings.fileSizeThreshold.setValue(m_fileSizeThresholdSpinBox->value());
    m_useCustomSettings = m_useCustomSettingsCheckBox->isChecked();
    settings.writeSettings();
}

void ClangFormatGlobalConfigWidget::cancel()
{
    // Only the global view changes the global "use custom settings" live, so
    // only it needs to restore the original value on cancel.
    if (m_project)
        return;
    clangFormatSettings().useCustomSettings.setValue(m_useCustomSettings);
}

ClangFormatSettings::Mode ClangFormatGlobalConfigWidget::mode() const
{
    if (m_project && m_useGlobalSettings->isChecked())
        return clangFormatSettings().mode();
    if (!m_useClangFormat->isChecked())
        return ClangFormatSettings::Mode::Disable;
    return static_cast<ClangFormatSettings::Mode>(m_indentingOrFormatting->currentIndex());
}

bool ClangFormatGlobalConfigWidget::useCustomSettings() const
{
    return m_useCustomSettingsCheckBox->isChecked();
}

} // namespace ClangFormat
