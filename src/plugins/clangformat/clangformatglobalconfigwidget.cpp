// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "clangformatglobalconfigwidget.h"

#include "clangformatconstants.h"
#include "clangformatsettings.h"
#include "clangformattr.h"
#include "clangformatutils.h"

#include <projectexplorer/project.h>
#include <texteditor/icodestylepreferences.h>

#include <utils/layoutbuilder.h>

#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QSpinBox>
#include <QWidget>

#include <sstream>

using namespace ProjectExplorer;
using namespace Utils;

namespace ClangFormat {

ClangFormatGlobalConfigWidget::ClangFormatGlobalConfigWidget(
    TextEditor::ICodeStylePreferences *codeStyle, ProjectExplorer::Project *project, QWidget *parent)
    : CppCodeStyleWidget(parent)
    , m_project(project)
    , m_codeStyle(codeStyle)
{
    const QString sizeThresholdToolTip = Tr::tr(
        "Files greater than this will not be indented by ClangFormat.\n"
        "The built-in code indenter will handle indentation.");

    m_projectHasClangFormat = new QLabel(this);
    m_formattingModeLabel = new QLabel(Tr::tr("Formatting mode:"));
    m_fileSizeThresholdLabel = new QLabel(Tr::tr("Ignore files greater than:"));
    m_fileSizeThresholdSpinBox = new QSpinBox(this);
    m_indentingOrFormatting = new QComboBox(this);
    m_formatWhileTyping = new QCheckBox(Tr::tr("Format while typing"));
    m_formatOnSave = new QCheckBox(Tr::tr("Format edited code on file save"));
    m_overrideDefault = new QCheckBox(Tr::tr("Override .clang-format file"));
    m_useGlobalSettings = new QCheckBox(Tr::tr("Use global settings"));
    m_useGlobalSettings->hide();
    m_overrideDefaultFile = ClangFormatSettings::instance().overrideDefaultFile();

    using namespace Layouting;

    QWidget *globalSettingsGroupBoxWidget = nullptr;

    Group globalSettingsGroupBox {
        bindTo(&globalSettingsGroupBoxWidget),
        title(Tr::tr("ClangFormat settings:")),
        Column {
            m_useGlobalSettings,
            Form {
                 m_formattingModeLabel, m_indentingOrFormatting, st, br,
                 m_fileSizeThresholdLabel, m_fileSizeThresholdSpinBox, st, br
            },
            m_formatWhileTyping,
            m_formatOnSave,
            m_projectHasClangFormat,
            m_overrideDefault
        }
    };

    Column {
        globalSettingsGroupBox,
        noMargin
    }.attachTo(this);

    initCheckBoxes();
    initIndentationOrFormattingCombobox();
    initOverrideCheckBox();
    initUseGlobalSettingsCheckBox();
    initFileSizeThresholdSpinBox();

    if (project) {
        m_formatOnSave->hide();
        m_formatWhileTyping->hide();

        m_useGlobalSettings->show();
        return;
    }

    globalSettingsGroupBoxWidget->show();
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
        m_formattingModeLabel->setDisabled(isDisabled);
        m_projectHasClangFormat->setDisabled(
            isDisabled
            || (m_indentingOrFormatting->currentIndex()
                == static_cast<int>(ClangFormatSettings::Mode::Disable)));
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

void ClangFormatGlobalConfigWidget::initFileSizeThresholdSpinBox()
{
    m_fileSizeThresholdSpinBox->setMinimum(1);
    m_fileSizeThresholdSpinBox->setMaximum(std::numeric_limits<int>::max());
    m_fileSizeThresholdSpinBox->setSuffix(" KB");
    m_fileSizeThresholdSpinBox->setValue(ClangFormatSettings::instance().fileSizeThreshold());
    if (m_project) {
        m_fileSizeThresholdSpinBox->hide();
        m_fileSizeThresholdLabel->hide();
    }

    connect(m_indentingOrFormatting, &QComboBox::currentIndexChanged, this, [this](int index) {
        m_fileSizeThresholdLabel->setEnabled(
            index != static_cast<int>(ClangFormatSettings::Mode::Disable));
        m_fileSizeThresholdSpinBox->setEnabled(
            index != static_cast<int>(ClangFormatSettings::Mode::Disable));
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

    auto setTemporarilyReadOnly = [this]() {
        if (m_ignoreChanges.isLocked())
            return;
        Utils::GuardLocker locker(m_ignoreChanges);
        m_codeStyle->currentPreferences()->setTemporarilyReadOnly(!m_overrideDefault->isChecked());
        m_codeStyle->currentPreferences()->setIsAdditionalTabDisabled(!m_overrideDefault->isEnabled());
        ClangFormatSettings::instance().write();
        emit m_codeStyle->currentPreferencesChanged(m_codeStyle->currentPreferences());
    };

    auto setEnableOverrideCheckBox = [this, setTemporarilyReadOnly](int index) {
        bool isDisable = index == static_cast<int>(ClangFormatSettings::Mode::Disable);
        m_overrideDefault->setDisabled(isDisable);
        m_projectHasClangFormat->setDisabled(isDisable);
        setTemporarilyReadOnly();
    };

    setEnableOverrideCheckBox(m_indentingOrFormatting->currentIndex());
    connect(m_indentingOrFormatting, &QComboBox::currentIndexChanged,
            this, setEnableOverrideCheckBox);

    m_overrideDefault->setToolTip("<html>"
                                  + Tr::tr("When this option is enabled, ClangFormat will use a "
                                           "user-specified configuration from the widget below, "
                                           "instead of the project .clang-format file. You can "
                                           "customize the formatting options for your code by "
                                           "adjusting the settings in the widget. Note that any "
                                           "changes made there will only affect the current "
                                           "configuration, and will not modify the project "
                                           ".clang-format file."));

    m_overrideDefault->setChecked(getProjectOverriddenSettings(m_project));
    setTemporarilyReadOnly();

    connect(m_overrideDefault, &QCheckBox::toggled, this, [this, setTemporarilyReadOnly](bool checked) {
        if (m_project) {
            m_project->setNamedSettings(Constants::OVERRIDE_FILE_ID, checked);
        } else {
            ClangFormatSettings::instance().setOverrideDefaultFile(checked);
            setTemporarilyReadOnly();
        }
    });

    connect(m_codeStyle,
            &TextEditor::ICodeStylePreferences::currentPreferencesChanged,
            this,
            setTemporarilyReadOnly);
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
        settings.setFileSizeThreshold(m_fileSizeThresholdSpinBox->value());
        m_overrideDefaultFile = m_overrideDefault->isChecked();
    }
    settings.write();
}

void ClangFormatGlobalConfigWidget::finish()
{
    ClangFormatSettings::instance().setOverrideDefaultFile(m_overrideDefaultFile);
    m_codeStyle->currentPreferences()->setTemporarilyReadOnly(
        !ClangFormatSettings::instance().overrideDefaultFile());
}

} // namespace ClangFormat
