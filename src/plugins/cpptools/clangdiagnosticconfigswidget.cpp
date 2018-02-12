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
#include "ui_clangbasechecks.h"
#include "ui_clazychecks.h"
#include "ui_tidychecks.h"

#include <utils/algorithm.h>
#include <utils/qtcassert.h>
#include <utils/utilsicons.h>

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
    setupTabs();

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
        m_clangBaseChecks->diagnosticOptionsTextEdit->setFocus();
    }
}

void ClangDiagnosticConfigsWidget::onRemoveButtonClicked()
{
    m_diagnosticConfigsModel.removeConfigWithId(currentConfigId());
    emit customConfigsChanged(customConfigs());

    syncConfigChooserToModel();
}

void ClangDiagnosticConfigsWidget::onClangTidyItemChanged(QListWidgetItem *item)
{
    const QString prefix = item->text();
    ClangDiagnosticConfig config = currentConfig();
    QString checks = config.clangTidyChecks();
    item->checkState() == Qt::Checked
            ? checks.append(',' + prefix)
            : checks.remove(',' + prefix);
    config.setClangTidyChecks(checks);
    updateConfig(config);
}

void ClangDiagnosticConfigsWidget::onClazyRadioButtonChanged(bool checked)
{
    if (!checked)
        return;

    QString checks;
    if (m_clazyChecks->clazyRadioDisabled->isChecked())
        checks = QString();
    else if (m_clazyChecks->clazyRadioLevel0->isChecked())
        checks = "level0";
    else if (m_clazyChecks->clazyRadioLevel1->isChecked())
        checks = "level1";
    else if (m_clazyChecks->clazyRadioLevel2->isChecked())
        checks = "level2";
    else if (m_clazyChecks->clazyRadioLevel3->isChecked())
        checks = "level3";

    ClangDiagnosticConfig config = currentConfig();
    config.setClazyChecks(checks);
    updateConfig(config);
}

static bool isAcceptedWarningOption(const QString &option)
{
    return option == "-w"
        || option == "-pedantic"
        || option == "-pedantic-errors";
}

// Reference:
// https://gcc.gnu.org/onlinedocs/gcc/Warning-Options.html
// https://clang.llvm.org/docs/DiagnosticsReference.html
static bool isValidOption(const QString &option)
{
    if (option == "-Werror")
        return false; // Avoid errors due to unknown or misspelled warnings.
    return option.startsWith("-W") || isAcceptedWarningOption(option);
}

static QString validateDiagnosticOptions(const QStringList &options)
{
    // This is handy for testing, allow disabling validation.
    if (qEnvironmentVariableIntValue("QTC_CLANG_NO_DIAGNOSTIC_CHECK"))
        return QString();

    for (const QString &option : options) {
        if (!isValidOption(option))
            return ClangDiagnosticConfigsWidget::tr("Option \"%1\" is invalid.").arg(option);
    }

    return QString();
}

static QStringList normalizeDiagnosticInputOptions(const QString &options)
{
    return options.simplified().split(QLatin1Char(' '), QString::SkipEmptyParts);
}

void ClangDiagnosticConfigsWidget::onDiagnosticOptionsEdited()
{
    // Clean up input
    const QString diagnosticOptions = m_clangBaseChecks->diagnosticOptionsTextEdit->document()
                                          ->toPlainText();
    const QStringList normalizedOptions = normalizeDiagnosticInputOptions(diagnosticOptions);

    // Validate
    const QString errorMessage = validateDiagnosticOptions(normalizedOptions);
    updateValidityWidgets(errorMessage);
    if (!errorMessage.isEmpty()) {
        // Remember the entered options in case the user will switch back.
        m_notAcceptedOptions.insert(currentConfigId(), diagnosticOptions);
        return;
    }
    m_notAcceptedOptions.remove(currentConfigId());

    // Commit valid changes
    ClangDiagnosticConfig updatedConfig = currentConfig();
    updatedConfig.setClangOptions(normalizedOptions);
    updateConfig(updatedConfig);
}

void ClangDiagnosticConfigsWidget::syncWidgetsToModel(const Core::Id &configToSelect)
{
    syncConfigChooserToModel(configToSelect);
    syncOtherWidgetsToComboBox();
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
                = ClangDiagnosticConfigsModel::displayNameWithBuiltinIndication(config);
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

    // Update Text Edit
    const QString options = m_notAcceptedOptions.contains(config.id())
            ? m_notAcceptedOptions.value(config.id())
            : config.clangOptions().join(QLatin1Char(' '));
    setDiagnosticOptions(options);
    m_clangBaseChecksWidget->setEnabled(!config.isReadOnly());

    if (config.isReadOnly()) {
        m_ui->infoIcon->setPixmap(Utils::Icons::INFO.pixmap());
        m_ui->infoLabel->setText(tr("Copy this configuration to customize it."));
        m_ui->infoLabel->setStyleSheet(QString());
    }

    syncClangTidyWidgets(config);
    syncClazyWidgets(config);
}

void ClangDiagnosticConfigsWidget::syncClangTidyWidgets(const ClangDiagnosticConfig &config)
{
    disconnectClangTidyItemChanged();

    const QString tidyChecks = config.clangTidyChecks();
    for (int row = 0; row < m_tidyChecks->checksList->count(); ++row) {
        QListWidgetItem *item = m_tidyChecks->checksList->item(row);

        Qt::ItemFlags flags = item->flags();
        flags |= Qt::ItemIsUserCheckable;
        if (config.isReadOnly())
            flags &= ~Qt::ItemIsEnabled;
        else
            flags |= Qt::ItemIsEnabled;
        item->setFlags(flags);

        if (tidyChecks.indexOf(item->text()) != -1)
            item->setCheckState(Qt::Checked);
        else
            item->setCheckState(Qt::Unchecked);
    }

    connectClangTidyItemChanged();
}

void ClangDiagnosticConfigsWidget::syncClazyWidgets(const ClangDiagnosticConfig &config)
{
    const QString clazyChecks = config.clazyChecks();

    QRadioButton *button = m_clazyChecks->clazyRadioDisabled;
    if (clazyChecks.isEmpty())
        button = m_clazyChecks->clazyRadioDisabled;
    else if (clazyChecks == "level0")
        button = m_clazyChecks->clazyRadioLevel0;
    else if (clazyChecks == "level1")
        button = m_clazyChecks->clazyRadioLevel1;
    else if (clazyChecks == "level2")
        button = m_clazyChecks->clazyRadioLevel2;
    else if (clazyChecks == "level3")
        button = m_clazyChecks->clazyRadioLevel3;

    button->setChecked(true);
    m_clazyChecksWidget->setEnabled(!config.isReadOnly());
}

void ClangDiagnosticConfigsWidget::updateConfig(const ClangDiagnosticConfig &config)
{
    m_diagnosticConfigsModel.appendOrUpdate(config);
    emit customConfigsChanged(customConfigs());
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
    if (options != m_clangBaseChecks->diagnosticOptionsTextEdit->document()->toPlainText()) {
        disconnectDiagnosticOptionsChanged();
        m_clangBaseChecks->diagnosticOptionsTextEdit->document()->setPlainText(options);
        connectDiagnosticOptionsChanged();
    }

    const QString errorMessage
            = validateDiagnosticOptions(normalizeDiagnosticInputOptions(options));
    updateValidityWidgets(errorMessage);
}

void ClangDiagnosticConfigsWidget::updateValidityWidgets(const QString &errorMessage)
{
    QString validationResult;
    const Utils::Icon *icon = nullptr;
    QString styleSheet;
    if (errorMessage.isEmpty()) {
        icon = &Utils::Icons::INFO;
        validationResult = tr("Configuration passes sanity checks.");
    } else {
        icon = &Utils::Icons::CRITICAL;
        validationResult = tr("%1").arg(errorMessage);
        styleSheet = "color: red;";
    }

    m_ui->infoIcon->setPixmap(icon->pixmap());
    m_ui->infoLabel->setText(validationResult);
    m_ui->infoLabel->setStyleSheet(styleSheet);
}

void ClangDiagnosticConfigsWidget::connectClangTidyItemChanged()
{
    connect(m_tidyChecks->checksList, &QListWidget::itemChanged,
            this, &ClangDiagnosticConfigsWidget::onClangTidyItemChanged);
}

void ClangDiagnosticConfigsWidget::disconnectClangTidyItemChanged()
{
    disconnect(m_tidyChecks->checksList, &QListWidget::itemChanged,
               this, &ClangDiagnosticConfigsWidget::onClangTidyItemChanged);
}

void ClangDiagnosticConfigsWidget::connectClazyRadioButtonClicked(QRadioButton *button)
{
    connect(button,
            &QRadioButton::clicked,
            this,
            &ClangDiagnosticConfigsWidget::onClazyRadioButtonChanged);
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
    connect(m_clangBaseChecks->diagnosticOptionsTextEdit->document(),
            &QTextDocument::contentsChanged,
            this,
            &ClangDiagnosticConfigsWidget::onDiagnosticOptionsEdited);
}

void ClangDiagnosticConfigsWidget::disconnectDiagnosticOptionsChanged()
{
    disconnect(m_clangBaseChecks->diagnosticOptionsTextEdit->document(),
               &QTextDocument::contentsChanged,
               this,
               &ClangDiagnosticConfigsWidget::onDiagnosticOptionsEdited);
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

void ClangDiagnosticConfigsWidget::setupTabs()
{
    m_clangBaseChecks.reset(new CppTools::Ui::ClangBaseChecks);
    m_clangBaseChecksWidget = new QWidget();
    m_clangBaseChecks->setupUi(m_clangBaseChecksWidget);

    m_clazyChecks.reset(new CppTools::Ui::ClazyChecks);
    m_clazyChecksWidget = new QWidget();
    m_clazyChecks->setupUi(m_clazyChecksWidget);

    connectClazyRadioButtonClicked(m_clazyChecks->clazyRadioDisabled);
    connectClazyRadioButtonClicked(m_clazyChecks->clazyRadioLevel0);
    connectClazyRadioButtonClicked(m_clazyChecks->clazyRadioLevel1);
    connectClazyRadioButtonClicked(m_clazyChecks->clazyRadioLevel2);
    connectClazyRadioButtonClicked(m_clazyChecks->clazyRadioLevel3);

    m_tidyChecks.reset(new CppTools::Ui::TidyChecks);
    m_tidyChecksWidget = new QWidget();
    m_tidyChecks->setupUi(m_tidyChecksWidget);
    connectClangTidyItemChanged();

    m_ui->tabWidget->addTab(m_clangBaseChecksWidget, tr("Clang"));
    m_ui->tabWidget->addTab(m_tidyChecksWidget, tr("Clang-Tidy"));
    m_ui->tabWidget->addTab(m_clazyChecksWidget, tr("Clazy"));
    m_ui->tabWidget->setCurrentIndex(0);
}

} // CppTools namespace
