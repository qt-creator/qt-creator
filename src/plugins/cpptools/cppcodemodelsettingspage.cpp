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

#include "cppcodemodelsettingspage.h"

#include "clangdiagnosticconfigswidget.h"
#include "cppmodelmanager.h"
#include "cpptoolsconstants.h"
#include "ui_cppcodemodelsettingspage.h"
#include "ui_clazychecks.h"
#include "ui_tidychecks.h"

#include <coreplugin/icore.h>
#include <utils/algorithm.h>

#include <QTextStream>

using namespace CppTools;
using namespace CppTools::Internal;

CppCodeModelSettingsWidget::CppCodeModelSettingsWidget(QWidget *parent)
    : QWidget(parent)
    , m_ui(new Ui::CppCodeModelSettingsPage)
{
    m_ui->setupUi(this);

    m_ui->clangSettingsGroupBox->setVisible(true);
}

CppCodeModelSettingsWidget::~CppCodeModelSettingsWidget()
{
    delete m_ui;
}

void CppCodeModelSettingsWidget::setSettings(const QSharedPointer<CppCodeModelSettings> &s)
{
    m_settings = s;

    setupGeneralWidgets();
    setupClangCodeModelWidgets();
}

void CppCodeModelSettingsWidget::applyToSettings() const
{
    bool changed = false;

    changed |= applyGeneralWidgetsToSettings();
    changed |= applyClangCodeModelWidgetsToSettings();

    if (changed)
        m_settings->toSettings(Core::ICore::settings());
}

void CppCodeModelSettingsWidget::setupClangCodeModelWidgets()
{
    const bool isClangActive = CppModelManager::instance()->isClangCodeModelActive();

    m_ui->clangCodeModelIsDisabledHint->setVisible(!isClangActive);
    m_ui->clangCodeModelIsEnabledHint->setVisible(isClangActive);
    m_ui->clangSettingsGroupBox->setEnabled(isClangActive);

    ClangDiagnosticConfigsModel diagnosticConfigsModel(m_settings->clangCustomDiagnosticConfigs());
    m_clangDiagnosticConfigsWidget = new ClangDiagnosticConfigsWidget(
                                            diagnosticConfigsModel,
                                            m_settings->clangDiagnosticConfigId());
    m_ui->clangSettingsGroupBox->layout()->addWidget(m_clangDiagnosticConfigsWidget);

    m_ui->clangPlugins->setEnabled(isClangActive);
    setupPluginsWidgets();
}

void CppCodeModelSettingsWidget::setupPluginsWidgets()
{
    m_clazyChecks.reset(new CppTools::Ui::ClazyChecks);
    m_clazyChecksWidget = new QWidget();
    m_clazyChecks->setupUi(m_clazyChecksWidget);

    m_tidyChecks.reset(new CppTools::Ui::TidyChecks);
    m_tidyChecksWidget = new QWidget();
    m_tidyChecks->setupUi(m_tidyChecksWidget);

    m_ui->pluginChecks->addWidget(m_tidyChecksWidget);
    m_ui->pluginChecks->addWidget(m_clazyChecksWidget);
    m_ui->pluginChecks->setCurrentIndex(0);

    connect(m_ui->pluginSelection,
            static_cast<void (QComboBox::*)(int index)>(&QComboBox::currentIndexChanged),
            [this](int index) {
        m_ui->pluginChecks->setCurrentIndex(index);
    });

    setupTidyChecks();
    setupClazyChecks();
}

void CppCodeModelSettingsWidget::setupTidyChecks()
{
    m_currentTidyChecks = m_settings->tidyChecks();
    for (int row = 0; row < m_tidyChecks->checksList->count(); ++row) {
        QListWidgetItem *item = m_tidyChecks->checksList->item(row);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        if (m_currentTidyChecks.indexOf(item->text()) != -1)
            item->setCheckState(Qt::Checked);
        else
            item->setCheckState(Qt::Unchecked);
    }
    connect(m_tidyChecks->checksList, &QListWidget::itemChanged, [this](QListWidgetItem *item) {
        const QString prefix = item->text();
        item->checkState() == Qt::Checked
                ? m_currentTidyChecks.append(',' + prefix)
                : m_currentTidyChecks.remove(',' + prefix);
    });
}

void CppCodeModelSettingsWidget::setupClazyChecks()
{
    // Levels descriptions are taken from https://github.com/KDE/clazy
    static const std::array<QString, 5> levelDescriptions {{
        QString(),
        tr("Very stable checks, 99.99% safe, no false-positives."),
        tr("Similar to level0, but sometimes (rarely) there might be\n"
           "some false-positives."),
        tr("Sometimes has false-positives (20-30%)."),
        tr("Not always correct, possibly very noisy, might require\n"
           "a knowledgeable developer to review, might have a very big\n"
           "rate of false-positives, might have bugs.")
    }};

    m_currentClazyChecks = m_settings->clazyChecks();
    if (!m_currentClazyChecks.isEmpty()) {
        m_clazyChecks->clazyLevel->setCurrentText(m_currentClazyChecks);
        const unsigned index = static_cast<unsigned>(m_clazyChecks->clazyLevel->currentIndex());
        m_clazyChecks->levelDescription->setText(levelDescriptions[index]);
    }

    connect(m_clazyChecks->clazyLevel,
            static_cast<void (QComboBox::*)(int index)>(&QComboBox::currentIndexChanged),
            [this](int index) {
        m_clazyChecks->levelDescription->setText(levelDescriptions[static_cast<unsigned>(index)]);
        if (index == 0) {
            m_currentClazyChecks.clear();
            return;
        }
        m_currentClazyChecks = m_clazyChecks->clazyLevel->itemText(index);
    });
}

void CppCodeModelSettingsWidget::setupGeneralWidgets()
{
    m_ui->interpretAmbiguousHeadersAsCHeaders->setChecked(
                m_settings->interpretAmbigiousHeadersAsCHeaders());

    m_ui->skipIndexingBigFilesCheckBox->setChecked(m_settings->skipIndexingBigFiles());
    m_ui->bigFilesLimitSpinBox->setValue(m_settings->indexerFileSizeLimitInMb());

    const bool ignorePch = m_settings->pchUsage() == CppCodeModelSettings::PchUse_None;
    m_ui->ignorePCHCheckBox->setChecked(ignorePch);
}

bool CppCodeModelSettingsWidget::applyClangCodeModelWidgetsToSettings() const
{
    bool settingsChanged = false;

    const Core::Id oldConfigId = m_settings->clangDiagnosticConfigId();
    const Core::Id currentConfigId = m_clangDiagnosticConfigsWidget->currentConfigId();
    if (oldConfigId != currentConfigId) {
        m_settings->setClangDiagnosticConfigId(currentConfigId);
        settingsChanged = true;
    }

    const ClangDiagnosticConfigs oldDiagnosticConfigs = m_settings->clangCustomDiagnosticConfigs();
    const ClangDiagnosticConfigs currentDiagnosticConfigs
            = m_clangDiagnosticConfigsWidget->customConfigs();
    if (oldDiagnosticConfigs != currentDiagnosticConfigs) {
        m_settings->setClangCustomDiagnosticConfigs(currentDiagnosticConfigs);
        settingsChanged = true;
    }

    if (m_settings->tidyChecks() != m_currentTidyChecks) {
        m_settings->setTidyChecks(m_currentTidyChecks);
        settingsChanged = true;
    }

    if (m_settings->clazyChecks() != m_currentClazyChecks) {
        m_settings->setClazyChecks(m_currentClazyChecks);
        settingsChanged = true;
    }

    return settingsChanged;
}

bool CppCodeModelSettingsWidget::applyGeneralWidgetsToSettings() const
{
    bool settingsChanged = false;

    const bool newInterpretAmbiguousHeaderAsCHeaders
            = m_ui->interpretAmbiguousHeadersAsCHeaders->isChecked();
    if (m_settings->interpretAmbigiousHeadersAsCHeaders()
            != newInterpretAmbiguousHeaderAsCHeaders) {
        m_settings->setInterpretAmbigiousHeadersAsCHeaders(newInterpretAmbiguousHeaderAsCHeaders);
        settingsChanged = true;
    }

    const bool newSkipIndexingBigFiles = m_ui->skipIndexingBigFilesCheckBox->isChecked();
    if (m_settings->skipIndexingBigFiles() != newSkipIndexingBigFiles) {
        m_settings->setSkipIndexingBigFiles(newSkipIndexingBigFiles);
        settingsChanged = true;
    }
    const int newFileSizeLimit = m_ui->bigFilesLimitSpinBox->value();
    if (m_settings->indexerFileSizeLimitInMb() != newFileSizeLimit) {
        m_settings->setIndexerFileSizeLimitInMb(newFileSizeLimit);
        settingsChanged = true;
    }

    const bool newIgnorePch = m_ui->ignorePCHCheckBox->isChecked();
    const bool previousIgnorePch = m_settings->pchUsage() == CppCodeModelSettings::PchUse_None;
    if (newIgnorePch != previousIgnorePch) {
        const CppCodeModelSettings::PCHUsage pchUsage = m_ui->ignorePCHCheckBox->isChecked()
                ? CppCodeModelSettings::PchUse_None
                : CppCodeModelSettings::PchUse_BuildSystem;
        m_settings->setPCHUsage(pchUsage);
        settingsChanged = true;
    }

    return settingsChanged;
}

CppCodeModelSettingsPage::CppCodeModelSettingsPage(QSharedPointer<CppCodeModelSettings> &settings,
                                                   QObject *parent)
    : Core::IOptionsPage(parent)
    , m_settings(settings)
{
    setId(Constants::CPP_CODE_MODEL_SETTINGS_ID);
    setDisplayName(QCoreApplication::translate("CppTools",Constants::CPP_CODE_MODEL_SETTINGS_NAME));
    setCategory(Constants::CPP_SETTINGS_CATEGORY);
    setDisplayCategory(QCoreApplication::translate("CppTools",Constants::CPP_SETTINGS_TR_CATEGORY));
    setCategoryIcon(Utils::Icon(Constants::SETTINGS_CATEGORY_CPP_ICON));
}

QWidget *CppCodeModelSettingsPage::widget()
{
    if (!m_widget) {
        m_widget = new CppCodeModelSettingsWidget;
        m_widget->setSettings(m_settings);
    }
    return m_widget;
}

void CppCodeModelSettingsPage::apply()
{
    if (m_widget)
        m_widget->applyToSettings();
}

void CppCodeModelSettingsPage::finish()
{
    delete m_widget;
}
