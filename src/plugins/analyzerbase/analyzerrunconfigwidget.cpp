/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
** Author: Milian Wolff, KDAB (milian.wolff@kdab.com)
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "analyzerrunconfigwidget.h"

#include <utils/detailswidget.h>
#include <utils/qtcassert.h>

#include <QDebug>
#include <QApplication>
#include <QLabel>
#include <QVBoxLayout>
#include <QComboBox>
#include <QPushButton>

namespace Analyzer {

AnalyzerRunConfigWidget::AnalyzerRunConfigWidget(ProjectExplorer::IRunConfigurationAspect *aspect)
{
    m_aspect = aspect;
    m_config = aspect->projectSettings();

    QWidget *globalSetting = new QWidget;
    QHBoxLayout *globalSettingLayout = new QHBoxLayout(globalSetting);
    globalSettingLayout->setContentsMargins(0, 0, 0, 0);

    m_settingsCombo = new QComboBox(globalSetting);
    m_settingsCombo->addItems(QStringList()
                            << QApplication::translate("ProjectExplorer::Internal::EditorSettingsPropertiesPage", "Global")
                            << QApplication::translate("ProjectExplorer::Internal::EditorSettingsPropertiesPage", "Custom")
                            );
    globalSettingLayout->addWidget(m_settingsCombo);
    connect(m_settingsCombo, static_cast<void (QComboBox::*)(int)>(&QComboBox::activated),
            this, &AnalyzerRunConfigWidget::chooseSettings);
    m_restoreButton = new QPushButton(
                QApplication::translate("ProjectExplorer::Internal::EditorSettingsPropertiesPage", "Restore Global"),
                globalSetting);
    globalSettingLayout->addWidget(m_restoreButton);
    connect(m_restoreButton, &QPushButton::clicked, this, &AnalyzerRunConfigWidget::restoreGlobal);
    globalSettingLayout->addStretch(2);

    QWidget *innerPane = new QWidget;
    m_configWidget = m_config->createConfigWidget(innerPane);

    QVBoxLayout *layout = new QVBoxLayout(innerPane);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(globalSetting);
    layout->addWidget(m_configWidget);

    m_details = new Utils::DetailsWidget;
    m_details->setWidget(innerPane);

    QVBoxLayout *outerLayout = new QVBoxLayout(this);
    outerLayout->addWidget(m_details);
    outerLayout->setContentsMargins(0, 0, 0, 0);

    chooseSettings(m_aspect->isUsingGlobalSettings() ? 0 : 1);
}

QString AnalyzerRunConfigWidget::displayName() const
{
    return m_aspect->displayName();
}

void AnalyzerRunConfigWidget::chooseSettings(int setting)
{
    QTC_ASSERT(m_aspect, return);
    bool isCustom = (setting == 1);

    m_settingsCombo->setCurrentIndex(setting);
    m_aspect->setUsingGlobalSettings(!isCustom);
    m_configWidget->setEnabled(isCustom);
    m_restoreButton->setEnabled(isCustom);
    m_details->setSummaryText(isCustom
        ? tr("Use Customized Settings")
        : tr("Use Global Settings"));
}

void AnalyzerRunConfigWidget::restoreGlobal()
{
    QTC_ASSERT(m_aspect, return);
    m_aspect->resetProjectToGlobalSettings();
}

} // namespace Analyzer
