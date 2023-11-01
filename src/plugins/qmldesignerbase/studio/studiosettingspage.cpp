// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "studiosettingspage.h"

#include "../utils/designerpaths.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/dialogs/restartdialog.h>
#include <coreplugin/icore.h>

#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>

#include <utils/qtcsettings.h>
#include <utils/theme/theme.h>

#include <QApplication>
#include <QCheckBox>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSpacerItem>
#include <QStyle>
#include <QStyleFactory>
#include <QVBoxLayout>

using namespace Utils;

namespace QmlDesigner {

namespace {

bool hideBuildMenuSetting()
{
    return Core::ICore::settings()
        ->value(ProjectExplorer::Constants::SETTINGS_MENU_HIDE_BUILD, false)
        .toBool();
}

bool hideDebugMenuSetting()
{
    return Core::ICore::settings()
        ->value(ProjectExplorer::Constants::SETTINGS_MENU_HIDE_DEBUG, false)
        .toBool();
}

bool hideAnalyzeMenuSetting()
{
    return Core::ICore::settings()
        ->value(ProjectExplorer::Constants::SETTINGS_MENU_HIDE_ANALYZE, false)
        .toBool();
}

bool hideToolsMenuSetting()
{
    return Core::ICore::settings()->value(Core::Constants::SETTINGS_MENU_HIDE_TOOLS, false).toBool();
}

void setSettingIfDifferent(const Key &key, bool value, bool &dirty)
{
    QtcSettings *s = Core::ICore::settings();
    if (s->value(key, false).toBool() != value) {
        dirty = true;
        s->setValue(key, value);
    }
}

} // namespace

StudioSettingsPage::StudioSettingsPage()
    : m_buildCheckBox(new QCheckBox(tr("Build")))
    , m_debugCheckBox(new QCheckBox(tr("Debug")))
    , m_analyzeCheckBox(new QCheckBox(tr("Analyze")))
    , m_toolsCheckBox(new QCheckBox(tr("Tools")))
    , m_pathChooserExamples(new Utils::PathChooser())
    , m_pathChooserBundles(new Utils::PathChooser())
{
    const QString toolTip = tr(
        "Hide top-level menus with advanced functionality to simplify the UI. <b>Build</b> is "
        "generally not required in the context of Qt Design Studio. <b>Debug</b> and "
        "<b>Analyze</b> "
        "are only required for debugging and profiling. <b>Tools</b> can be useful for bookmarks "
        "and git integration.");

    QVBoxLayout *boxLayout = new QVBoxLayout();
    setLayout(boxLayout);
    auto groupBox = new QGroupBox(tr("Hide Menu"));
    groupBox->setToolTip(toolTip);
    boxLayout->addWidget(groupBox);

    auto verticalLayout = new QVBoxLayout();
    groupBox->setLayout(verticalLayout);

    m_buildCheckBox->setToolTip(toolTip);
    m_debugCheckBox->setToolTip(toolTip);
    m_analyzeCheckBox->setToolTip(toolTip);
    m_toolsCheckBox->setToolTip(toolTip);

    verticalLayout->addWidget(m_buildCheckBox);
    verticalLayout->addWidget(m_debugCheckBox);
    verticalLayout->addWidget(m_analyzeCheckBox);
    verticalLayout->addWidget(m_toolsCheckBox);

    verticalLayout->addSpacerItem(
        new QSpacerItem(10, 10, QSizePolicy::Expanding, QSizePolicy::Minimum));

    m_buildCheckBox->setChecked(hideBuildMenuSetting());
    m_debugCheckBox->setChecked(hideDebugMenuSetting());
    m_analyzeCheckBox->setChecked(hideAnalyzeMenuSetting());
    m_toolsCheckBox->setChecked(hideToolsMenuSetting());

    // Examples path setting
    auto examplesGroupBox = new QGroupBox(tr("Examples"));
    boxLayout->addWidget(examplesGroupBox);

    auto examplesLayout = new QHBoxLayout(this);
    examplesGroupBox->setLayout(examplesLayout);

    auto examplesLabel = new QLabel(tr("Examples path:"));
    m_pathChooserExamples->setFilePath(Utils::FilePath::fromString(Paths::examplesPathSetting()));
    auto examplesResetButton = new QPushButton(tr("Reset Path"));

    connect(examplesResetButton, &QPushButton::clicked, this, [this]() {
        m_pathChooserExamples->setFilePath(Paths::defaultExamplesPath());
    });

    examplesLayout->addWidget(examplesLabel);
    examplesLayout->addWidget(m_pathChooserExamples);
    examplesLayout->addWidget(examplesResetButton);

    // Bundles path setting
    auto bundlesGroupBox = new QGroupBox(tr("Bundles"));
    boxLayout->addWidget(bundlesGroupBox);

    auto bundlesLayout = new QHBoxLayout(this);
    bundlesGroupBox->setLayout(bundlesLayout);

    QLabel *bundlesLabel = new QLabel(tr("Bundles path:"));
    m_pathChooserBundles->setFilePath(Utils::FilePath::fromString(Paths::bundlesPathSetting()));
    QPushButton *bundlesResetButton = new QPushButton(tr("Reset Path"));

    connect(bundlesResetButton, &QPushButton::clicked, this, [this]() {
        m_pathChooserBundles->setFilePath(Paths::defaultBundlesPath());
    });

    bundlesLayout->addWidget(bundlesLabel);
    bundlesLayout->addWidget(m_pathChooserBundles);
    bundlesLayout->addWidget(bundlesResetButton);

    boxLayout->addSpacerItem(
        new QSpacerItem(10, 10, QSizePolicy::Expanding, QSizePolicy::Expanding));
}

void StudioSettingsPage::apply()
{
    bool dirty = false;

    setSettingIfDifferent(ProjectExplorer::Constants::SETTINGS_MENU_HIDE_BUILD,
                          m_buildCheckBox->isChecked(),
                          dirty);

    setSettingIfDifferent(ProjectExplorer::Constants::SETTINGS_MENU_HIDE_DEBUG,
                          m_debugCheckBox->isChecked(),
                          dirty);

    setSettingIfDifferent(ProjectExplorer::Constants::SETTINGS_MENU_HIDE_ANALYZE,
                          m_analyzeCheckBox->isChecked(),
                          dirty);

    setSettingIfDifferent(Core::Constants::SETTINGS_MENU_HIDE_TOOLS,
                          m_toolsCheckBox->isChecked(),
                          dirty);

    if (dirty) {
        const QString restartText = tr(
            "The menu visibility change will take effect after restart.");
        Core::RestartDialog restartDialog(Core::ICore::dialogParent(), restartText);
        restartDialog.exec();
    }

    QtcSettings *s = Core::ICore::settings();
    const QString value = m_pathChooserExamples->filePath().toString();

    if (s->value(Paths::exampleDownloadPath, false).toString() != value) {
        s->setValue(Paths::exampleDownloadPath, value);
        emit examplesDownloadPathChanged(value);
    }

    const QString bundlesPath = m_pathChooserBundles->filePath().toString();

    if (s->value(Paths::bundlesDownloadPath).toString() != bundlesPath) {
        s->setValue(Paths::bundlesDownloadPath, bundlesPath);
        emit bundlesDownloadPathChanged(bundlesPath);

        const QString restartText = tr("Changing bundle path will take effect after restart.");
        Core::RestartDialog restartDialog(Core::ICore::dialogParent(), restartText);
        restartDialog.exec();
    }
}

StudioConfigSettingsPage::StudioConfigSettingsPage()
{
    setId("Z.StudioConfig.Settings");
    setDisplayName(tr("Qt Design Studio Configuration"));
    setCategory(Core::Constants::SETTINGS_CATEGORY_CORE);
    setWidgetCreator([&] {
        auto page = new StudioSettingsPage;
        QObject::connect(page,
                &StudioSettingsPage::examplesDownloadPathChanged,
                this,
                &StudioConfigSettingsPage::examplesDownloadPathChanged);
        QObject::connect(page,
                &StudioSettingsPage::bundlesDownloadPathChanged,
                this,
                &StudioConfigSettingsPage::bundlesDownloadPathChanged);
        return page;
    });
}

} // namespace QmlDesigner
