// Copyright (C) 2020 Leander Schulten <Leander.Schulten@rwth-aachen.de>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cppquickfixprojectsettingswidget.h"

#include "cppeditorconstants.h"
#include "cppeditortr.h"
#include "cppquickfixsettingswidget.h"

#include <QFile>
#include <QGridLayout>
#include <QPushButton>

namespace CppEditor::Internal {

CppQuickFixProjectSettingsWidget::CppQuickFixProjectSettingsWidget(ProjectExplorer::Project *project,
                                                                   QWidget *parent)
    : ProjectExplorer::ProjectSettingsWidget(parent)
{
    setGlobalSettingsId(CppEditor::Constants::QUICK_FIX_SETTINGS_ID);
    m_projectSettings = CppQuickFixProjectsSettings::getSettings(project);

    m_pushButton = new QPushButton(this);

    auto gridLayout = new QGridLayout(this);
    gridLayout->setContentsMargins(0, 0, 0, 0);
    gridLayout->addWidget(m_pushButton, 1, 0, 1, 1);
    auto layout = new QVBoxLayout();
    gridLayout->addLayout(layout, 2, 0, 1, 2);

    m_settingsWidget = new CppQuickFixSettingsWidget;
    m_settingsWidget->loadSettings(m_projectSettings->getSettings());

    if (QLayout *layout = m_settingsWidget->layout())
        layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_settingsWidget);

    connect(this, &ProjectSettingsWidget::useGlobalSettingsChanged,
            this, &CppQuickFixProjectSettingsWidget::currentItemChanged);
    setUseGlobalSettings(m_projectSettings->isUsingGlobalSettings());
    currentItemChanged(m_projectSettings->useCustomSettings());

    connect(m_pushButton, &QAbstractButton::clicked,
            this, &CppQuickFixProjectSettingsWidget::buttonCustomClicked);
    connect(m_settingsWidget, &CppQuickFixSettingsWidget::settingsChanged, this,
            [this] {
                m_settingsWidget->saveSettings(m_projectSettings->getSettings());
                if (!useGlobalSettings())
                    m_projectSettings->saveOwnSettings();
            });
}

CppQuickFixProjectSettingsWidget::~CppQuickFixProjectSettingsWidget() = default;

void CppQuickFixProjectSettingsWidget::currentItemChanged(bool useGlobalSettings)
{
    if (useGlobalSettings) {
        const auto &path = m_projectSettings->filePathOfSettingsFile();
        m_pushButton->setToolTip(Tr::tr("Custom settings are saved in a file. If you use the "
                                        "global settings, you can delete that file."));
        m_pushButton->setText(Tr::tr("Delete Custom Settings File"));
        m_pushButton->setVisible(!path.isEmpty() && path.exists());
        m_projectSettings->useGlobalSettings();
    } else /*Custom*/ {
        if (!m_projectSettings->useCustomSettings()) {
            setUseGlobalSettings(!m_projectSettings->useCustomSettings());
            return;
        }
        m_pushButton->setToolTip(Tr::tr("Resets all settings to the global settings."));
        m_pushButton->setText(Tr::tr("Reset to Global"));
        m_pushButton->setVisible(true);
        // otherwise you change the comboBox and exit and have no custom settings:
        m_projectSettings->saveOwnSettings();
    }
    m_settingsWidget->loadSettings(m_projectSettings->getSettings());
}

void CppQuickFixProjectSettingsWidget::buttonCustomClicked()
{
    if (useGlobalSettings()) {
        // delete file
        QFile::remove(m_projectSettings->filePathOfSettingsFile().toString());
        m_pushButton->setVisible(false);
    } else /*Custom*/ {
        m_projectSettings->resetOwnSettingsToGlobal();
        m_projectSettings->saveOwnSettings();
        m_settingsWidget->loadSettings(m_projectSettings->getSettings());
    }
}

} // CppEditor::Internal
