/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
** Author: Milian Wolff, KDAB (milian.wolff@kdab.com)
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "analyzerrunconfigwidget.h"

#include <utils/qtcassert.h>

#include <QDebug>
#include <QApplication>
#include <QLabel>
#include <QVBoxLayout>
#include <QComboBox>
#include <QPushButton>

namespace Analyzer {
namespace Internal {

AnalyzerToolDetailWidget::AnalyzerToolDetailWidget(AbstractAnalyzerSubConfig *config, QWidget *parent)
    : Utils::DetailsWidget(parent)
{
    QTC_ASSERT(config!=0, return);

    // update summary text
    setSummaryText(tr("<strong>%1</strong> settings").arg(config->displayName()));

    // create config widget
    QWidget *configWidget = config->createConfigWidget(this);
    setWidget(configWidget);
}

AnalyzerRunConfigWidget::AnalyzerRunConfigWidget()
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    QWidget *globalSetting = new QWidget(this);
    QHBoxLayout *globalSettingLayout = new QHBoxLayout(globalSetting);
    globalSettingLayout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(globalSetting);
    QLabel *label = new QLabel(tr("Analyzer settings:"), globalSetting);
    globalSettingLayout->addWidget(label);
    m_settingsCombo = new QComboBox(globalSetting);
    m_settingsCombo->addItems(QStringList()
                            << QApplication::translate("ProjectExplorer::Internal::EditorSettingsPropertiesPage", "Global")
                            << QApplication::translate("ProjectExplorer::Internal::EditorSettingsPropertiesPage", "Custom")
                            );
    globalSettingLayout->addWidget(m_settingsCombo);
    connect(m_settingsCombo, SIGNAL(activated(int)), this, SLOT(chooseSettings(int)));
    m_restoreButton = new QPushButton(
                QApplication::translate("ProjectExplorer::Internal::EditorSettingsPropertiesPage", "Restore Global"),
                globalSetting);
    globalSettingLayout->addWidget(m_restoreButton);
    connect(m_restoreButton, SIGNAL(clicked()), this, SLOT(restoreGlobal()));
    globalSettingLayout->addStretch(2);

    m_subConfigWidget = new QWidget(this);
    QVBoxLayout *subConfigLayout = new QVBoxLayout(m_subConfigWidget);
    subConfigLayout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_subConfigWidget);
}

QString AnalyzerRunConfigWidget::displayName() const
{
    return tr("Analyzer Settings");
}

void AnalyzerRunConfigWidget::setRunConfiguration(ProjectExplorer::RunConfiguration *rc)
{
    QTC_ASSERT(rc, return);

    m_aspect = rc->extraAspect<AnalyzerRunConfigurationAspect>();
    QTC_ASSERT(m_aspect, return);

    // add config widget for each sub config
    foreach (AbstractAnalyzerSubConfig *config, m_aspect->customSubConfigs()) {
        QWidget *widget = new AnalyzerToolDetailWidget(config);
        m_subConfigWidget->layout()->addWidget(widget);
    }
    setDetailEnabled(!m_aspect->isUsingGlobalSettings());
    m_settingsCombo->setCurrentIndex(m_aspect->isUsingGlobalSettings() ? 0 : 1);
    m_restoreButton->setEnabled(!m_aspect->isUsingGlobalSettings());
}

void AnalyzerRunConfigWidget::setDetailEnabled(bool value)
{
    QList<AnalyzerToolDetailWidget*> details = findChildren<AnalyzerToolDetailWidget*>();
    foreach (AnalyzerToolDetailWidget *detail, details)
        detail->widget()->setEnabled(value);
}

void AnalyzerRunConfigWidget::chooseSettings(int setting)
{
    QTC_ASSERT(m_aspect, return);
    setDetailEnabled(setting != 0);
    m_aspect->setUsingGlobalSettings(setting == 0);
    m_restoreButton->setEnabled(!m_aspect->isUsingGlobalSettings());
}

void AnalyzerRunConfigWidget::restoreGlobal()
{
    QTC_ASSERT(m_aspect, return);
    m_aspect->resetCustomToGlobalSettings();
}

} // namespace Internal
} // namespace Analyzer
