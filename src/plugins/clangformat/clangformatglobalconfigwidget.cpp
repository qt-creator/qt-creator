// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "clangformatglobalconfigwidget.h"

#include "clangformatconstants.h"
#include "clangformatsettings.h"
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
    m_formattingModeLabel = new QLabel(tr("Formatting mode:"));
    m_indentingOrFormatting = new QComboBox(this);
    m_formatWhileTyping = new QCheckBox(tr("Format while typing"));
    m_formatOnSave = new QCheckBox(tr("Format edited code on file save"));
    m_overrideDefault = new QCheckBox(tr("Override Clang Format configuration file"));

    using namespace Layouting;

    Group globalSettingsGroupBox {
        title(tr("ClangFormat settings:")),
            Column {
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

    if (project) {
        m_formattingModeLabel->hide();
        m_formatOnSave->hide();
        m_formatWhileTyping->hide();
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
                                        tr("Indenting only"));
    m_indentingOrFormatting->insertItem(static_cast<int>(ClangFormatSettings::Mode::Formatting),
                                        tr("Full formatting"));
    m_indentingOrFormatting->insertItem(static_cast<int>(ClangFormatSettings::Mode::Disable),
                                        tr("Disable"));

    if (m_project) {
        m_indentingOrFormatting->setCurrentIndex(
            m_project->namedSettings(Constants::MODE_ID).toInt());
    } else {
        m_indentingOrFormatting->setCurrentIndex(
            static_cast<int>(ClangFormatSettings::instance().mode()));
    }

    connect(m_indentingOrFormatting, &QComboBox::currentIndexChanged, this, [this](int index) {
        if (m_project)
            m_project->setNamedSettings(Constants::MODE_ID, index);
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
        m_projectHasClangFormat->setText(tr("The current project has its own .clang-format file which "
                                            "can be overridden by the settings below."));
    }

    auto setEnableOverrideCheckBox = [this](int index) {
        bool isDisable = index == static_cast<int>(ClangFormatSettings::Mode::Disable);
        m_overrideDefault->setEnabled(!isDisable);
    };

    setEnableOverrideCheckBox(m_indentingOrFormatting->currentIndex());
    connect(m_indentingOrFormatting, &QComboBox::currentIndexChanged,
            this, setEnableOverrideCheckBox);

    m_overrideDefault->setToolTip(
        tr("Override Clang Format configuration file with the chosen configuration."));

    if (m_project)
        m_overrideDefault->setChecked(
            m_project->namedSettings(Constants::OVERRIDE_FILE_ID).toBool());
    else
        m_overrideDefault->setChecked(ClangFormatSettings::instance().overrideDefaultFile());

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
