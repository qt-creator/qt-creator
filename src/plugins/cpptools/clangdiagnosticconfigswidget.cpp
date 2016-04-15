/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "clangdiagnosticconfigswidget.h"
#include "ui_clangdiagnosticconfigswidget.h"

#include <utils/algorithm.h>
#include <utils/qtcassert.h>

#include <QDebug>
#include <QInputDialog>
#include <QUuid>

namespace CppTools {

ClangDiagnosticConfigsWidget::ClangDiagnosticConfigsWidget(
        const ClangDiagnosticConfigsModel &diagnosticConfigsModel,
        const Core::Id &configToSelect,
        QWidget *parent)
    : QWidget(parent)
    , m_ui(new Ui::ClangDiagnosticConfigsWidget)
    , m_diagnosticConfigsModel(diagnosticConfigsModel)
{
    m_ui->setupUi(this);

    connectConfigChooserCurrentIndex();
    connect(m_ui->copyButton, &QPushButton::clicked,
            this, &ClangDiagnosticConfigsWidget::onCopyButtonClicked);
    connect(m_ui->removeButton, &QPushButton::clicked,
            this, &ClangDiagnosticConfigsWidget::onRemoveButtonClicked);
    connectDiagnosticOptionsChanged();

    syncWidgetsToModel(configToSelect);
}

ClangDiagnosticConfigsWidget::~ClangDiagnosticConfigsWidget()
{
    delete m_ui;
}

void ClangDiagnosticConfigsWidget::onCurrentConfigChanged(int)
{
    syncOtherWidgetsToComboBox();

    emit currentConfigChanged(currentConfigId());
}

static ClangDiagnosticConfig createCustomConfig(const ClangDiagnosticConfig &config,
                                                const QString &displayName)
{
    ClangDiagnosticConfig copied = config;
    copied.setId(Core::Id::fromString(QUuid::createUuid().toString()));
    copied.setDisplayName(displayName);
    copied.setIsReadOnly(false);

    return copied;
}

void ClangDiagnosticConfigsWidget::onCopyButtonClicked()
{
    const ClangDiagnosticConfig &config = currentConfig();

    bool diaglogAccepted = false;
    const QString newName = QInputDialog::getText(this,
                                                  tr("Copy Diagnostic Configuration"),
                                                  tr("Diagnostic configuration name:"),
                                                  QLineEdit::Normal,
                                                  tr("%1 (Copy)").arg(config.displayName()),
                                                  &diaglogAccepted);
    if (diaglogAccepted) {
        const ClangDiagnosticConfig customConfig = createCustomConfig(config, newName);
        m_diagnosticConfigsModel.appendOrUpdate(customConfig);
        emit customConfigsChanged(customConfigs());

        syncConfigChooserToModel(customConfig.id());
        m_ui->diagnosticOptionsTextEdit->setFocus();
    }
}

void ClangDiagnosticConfigsWidget::onRemoveButtonClicked()
{
    m_diagnosticConfigsModel.removeConfigWithId(currentConfigId());
    emit customConfigsChanged(customConfigs());

    syncConfigChooserToModel();
}

void ClangDiagnosticConfigsWidget::onDiagnosticOptionsEdited()
{
    const QString diagnosticOptions
            = m_ui->diagnosticOptionsTextEdit->document()->toPlainText().trimmed();
    const QStringList updatedCommandLine
            = diagnosticOptions.trimmed().split(QLatin1Char(' '), QString::SkipEmptyParts);

    ClangDiagnosticConfig updatedConfig = currentConfig();
    updatedConfig.setCommandLineOptions(updatedCommandLine);

    m_diagnosticConfigsModel.appendOrUpdate(updatedConfig);
    emit customConfigsChanged(customConfigs());
}

void ClangDiagnosticConfigsWidget::syncWidgetsToModel(const Core::Id &configToSelect)
{
    syncConfigChooserToModel(configToSelect);
    syncOtherWidgetsToComboBox();
}

static QString displayNameWithBuiltinIndication(const ClangDiagnosticConfig &config,
                                                const Core::Id &exceptionalConfig)
{
    if (exceptionalConfig == config.id())
        return config.displayName();

    return ClangDiagnosticConfigsModel::displayNameWithBuiltinIndication(config);
}

void ClangDiagnosticConfigsWidget::syncConfigChooserToModel(const Core::Id &configToSelect)
{
    disconnectConfigChooserCurrentIndex();

    const int previousCurrentIndex = m_ui->configChooserComboBox->currentIndex();
    m_ui->configChooserComboBox->clear();
    int configToSelectIndex = -1;

    const int size = m_diagnosticConfigsModel.size();
    for (int i = 0; i < size; ++i) {
        const ClangDiagnosticConfig &config = m_diagnosticConfigsModel.at(i);
        const QString displayName
                = displayNameWithBuiltinIndication(config, m_configWithUndecoratedDisplayName);
        m_ui->configChooserComboBox->addItem(displayName, config.id().toSetting());

        if (configToSelect == config.id())
            configToSelectIndex = i;
    }

    connectConfigChooserCurrentIndex();

    if (configToSelectIndex != -1) {
        m_ui->configChooserComboBox->setCurrentIndex(configToSelectIndex);
    } else if (previousCurrentIndex != m_ui->configChooserComboBox->currentIndex()) {
        syncOtherWidgetsToComboBox();
        emit currentConfigChanged(currentConfigId());
    }
}

void ClangDiagnosticConfigsWidget::syncOtherWidgetsToComboBox()
{
    if (isConfigChooserEmpty())
        return;

    const ClangDiagnosticConfig &config = currentConfig();

    // Update main button row
    m_ui->removeButton->setEnabled(!config.isReadOnly());

    // Update child widgets
    const QString commandLineOptions = config.commandLineOptions().join(QLatin1Char(' '));
    setDiagnosticOptions(commandLineOptions);
    m_ui->diagnosticOptionsTextEdit->setReadOnly(config.isReadOnly());
}

bool ClangDiagnosticConfigsWidget::isConfigChooserEmpty() const
{
    return m_ui->configChooserComboBox->count() == 0;
}

const ClangDiagnosticConfig &ClangDiagnosticConfigsWidget::currentConfig() const
{
    return m_diagnosticConfigsModel.configWithId(currentConfigId());
}

void ClangDiagnosticConfigsWidget::setDiagnosticOptions(const QString &options)
{
    if (options != m_ui->diagnosticOptionsTextEdit->document()->toPlainText()) {
        disconnectDiagnosticOptionsChanged();
        m_ui->diagnosticOptionsTextEdit->document()->setPlainText(options);
        connectDiagnosticOptionsChanged();
    }
}

void ClangDiagnosticConfigsWidget::connectConfigChooserCurrentIndex()
{
    connect(m_ui->configChooserComboBox,
            static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this,
            &ClangDiagnosticConfigsWidget::onCurrentConfigChanged);
}

void ClangDiagnosticConfigsWidget::disconnectConfigChooserCurrentIndex()
{
    disconnect(m_ui->configChooserComboBox,
               static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
               this,
               &ClangDiagnosticConfigsWidget::onCurrentConfigChanged);
}

void ClangDiagnosticConfigsWidget::connectDiagnosticOptionsChanged()
{
    connect(m_ui->diagnosticOptionsTextEdit->document(), &QTextDocument::contentsChanged,
               this, &ClangDiagnosticConfigsWidget::onDiagnosticOptionsEdited);
}

void ClangDiagnosticConfigsWidget::disconnectDiagnosticOptionsChanged()
{
    disconnect(m_ui->diagnosticOptionsTextEdit->document(), &QTextDocument::contentsChanged,
               this, &ClangDiagnosticConfigsWidget::onDiagnosticOptionsEdited);
}

void ClangDiagnosticConfigsWidget::setConfigWithUndecoratedDisplayName(const Core::Id &id)
{
    m_configWithUndecoratedDisplayName = id;
}

Core::Id ClangDiagnosticConfigsWidget::currentConfigId() const
{
    return Core::Id::fromSetting(m_ui->configChooserComboBox->currentData());
}

ClangDiagnosticConfigs ClangDiagnosticConfigsWidget::customConfigs() const
{
    const ClangDiagnosticConfigs allConfigs = m_diagnosticConfigsModel.configs();

    return Utils::filtered(allConfigs, [](const ClangDiagnosticConfig &config){
        return !config.isReadOnly();
    });
}

void ClangDiagnosticConfigsWidget::refresh(
        const ClangDiagnosticConfigsModel &diagnosticConfigsModel,
        const Core::Id &configToSelect)
{
    m_diagnosticConfigsModel = diagnosticConfigsModel;
    syncWidgetsToModel(configToSelect);
}

} // CppTools namespace
