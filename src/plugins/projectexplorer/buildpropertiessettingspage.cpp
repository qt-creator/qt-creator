/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "buildpropertiessettingspage.h"

#include "buildpropertiessettings.h"
#include "projectexplorer.h"

#include <QFormLayout>
#include <QComboBox>

namespace ProjectExplorer {
namespace Internal {

class BuildPropertiesSettingsPage::SettingsWidget : public QWidget
{
    Q_DECLARE_TR_FUNCTIONS(ProjectExplorer::Internal::BuildPropertiesSettingsPage)
public:
    SettingsWidget()
    {
        const BuildPropertiesSettings &settings = ProjectExplorerPlugin::buildPropertiesSettings();
        for (QComboBox * const comboBox : {&m_separateDebugInfoComboBox, &m_qmlDebuggingComboBox,
                                           &m_qtQuickCompilerComboBox}) {
            comboBox->addItem(tr("Enable"), TriState::Enabled.toVariant());
            comboBox->addItem(tr("Disable"),TriState::Disabled.toVariant());
            comboBox->addItem(tr("Use Project Default"), TriState::Default.toVariant());
        }
        m_separateDebugInfoComboBox.setCurrentIndex(m_separateDebugInfoComboBox
                                                .findData(settings.separateDebugInfo.toVariant()));
        m_qmlDebuggingComboBox.setCurrentIndex(m_qmlDebuggingComboBox
                                                .findData(settings.qmlDebugging.toVariant()));
        m_qtQuickCompilerComboBox.setCurrentIndex(m_qtQuickCompilerComboBox
                                                .findData(settings.qtQuickCompiler.toVariant()));
        const auto layout = new QFormLayout(this);
        layout->addRow(tr("Separate debug info:"), &m_separateDebugInfoComboBox);
        if (settings.showQtSettings) {
            layout->addRow(tr("QML debugging:"), &m_qmlDebuggingComboBox);
            layout->addRow(tr("Use Qt Quick Compiler:"), &m_qtQuickCompilerComboBox);
        } else {
            m_qmlDebuggingComboBox.hide();
            m_qtQuickCompilerComboBox.hide();
        }
    }

    BuildPropertiesSettings settings() const
    {
        BuildPropertiesSettings s;
        s.separateDebugInfo = TriState::fromVariant(m_separateDebugInfoComboBox.currentData());
        s.qmlDebugging = TriState::fromVariant(m_qmlDebuggingComboBox.currentData());
        s.qtQuickCompiler = TriState::fromVariant(m_qtQuickCompilerComboBox.currentData());
        return s;
    }

private:
    QComboBox m_separateDebugInfoComboBox;
    QComboBox m_qmlDebuggingComboBox;
    QComboBox m_qtQuickCompilerComboBox;
};

BuildPropertiesSettingsPage::BuildPropertiesSettingsPage()
{
    setId("AB.ProjectExplorer.BuildPropertiesSettingsPage");
    setDisplayName(tr("Default Build Properties"));
    setCategory(Constants::BUILD_AND_RUN_SETTINGS_CATEGORY);
}

QWidget *BuildPropertiesSettingsPage::widget()
{
    if (!m_widget)
        m_widget = new SettingsWidget;
    return m_widget;
}

void BuildPropertiesSettingsPage::apply()
{
    if (m_widget)
        ProjectExplorerPlugin::setBuildPropertiesSettings(m_widget->settings());
}

void BuildPropertiesSettingsPage::finish()
{
    delete m_widget;
}

} // namespace Internal
} // namespace ProjectExplorer
