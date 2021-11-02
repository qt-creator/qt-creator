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
#include "ui_cppcodemodelsettingspage.h"

#include "clangdiagnosticconfigswidget.h"
#include "cppeditorconstants.h"
#include "cppmodelmanager.h"
#include "cpptoolsreuse.h"

#include <coreplugin/icore.h>
#include <utils/algorithm.h>
#include <utils/infolabel.h>
#include <utils/pathchooser.h>

#include <QFormLayout>
#include <QSpinBox>
#include <QTextStream>
#include <QVersionNumber>

namespace CppEditor::Internal {

class CppCodeModelSettingsWidget final : public Core::IOptionsPageWidget
{
    Q_DECLARE_TR_FUNCTIONS(CppEditor::Internal::CppCodeModelSettingsWidget)

public:
    CppCodeModelSettingsWidget(CppCodeModelSettings *s);
    ~CppCodeModelSettingsWidget() override;

private:
    void apply() final;

    void setupGeneralWidgets();
    void setupClangCodeModelWidgets();

    bool applyGeneralWidgetsToSettings() const;
    bool applyClangCodeModelWidgetsToSettings() const;

    Ui::CppCodeModelSettingsPage *m_ui = nullptr;
    CppCodeModelSettings *m_settings = nullptr;
};

CppCodeModelSettingsWidget::CppCodeModelSettingsWidget(CppCodeModelSettings *s)
    : m_ui(new Ui::CppCodeModelSettingsPage)
{
    m_ui->setupUi(this);

    m_settings = s;

    setupGeneralWidgets();
    setupClangCodeModelWidgets();
}

CppCodeModelSettingsWidget::~CppCodeModelSettingsWidget()
{
    delete m_ui;
}

void CppCodeModelSettingsWidget::apply()
{
    bool changed = false;

    changed |= applyGeneralWidgetsToSettings();
    changed |= applyClangCodeModelWidgetsToSettings();

    if (changed)
        m_settings->toSettings(Core::ICore::settings());
}

void CppCodeModelSettingsWidget::setupClangCodeModelWidgets()
{
    m_ui->clangDiagnosticConfigsSelectionWidget
        ->refresh(diagnosticConfigsModel(),
                  m_settings->clangDiagnosticConfigId(),
                  [](const ClangDiagnosticConfigs &configs, const Utils::Id &configToSelect) {
                      return new ClangDiagnosticConfigsWidget(configs, configToSelect);
                  });

    const bool isClangActive = CppModelManager::instance()->isClangCodeModelActive();
    m_ui->clangCodeModelIsDisabledHint->setVisible(!isClangActive);
    m_ui->clangCodeModelIsEnabledHint->setVisible(isClangActive);

    for (int i = 0; i < m_ui->clangDiagnosticConfigsSelectionWidget->layout()->count(); ++i) {
        QWidget *widget = m_ui->clangDiagnosticConfigsSelectionWidget->layout()->itemAt(i)->widget();
        if (widget)
            widget->setEnabled(isClangActive);
    }
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
    bool changed = false;

    const Utils::Id oldConfigId = m_settings->clangDiagnosticConfigId();
    const Utils::Id currentConfigId = m_ui->clangDiagnosticConfigsSelectionWidget->currentConfigId();
    if (oldConfigId != currentConfigId) {
        m_settings->setClangDiagnosticConfigId(currentConfigId);
        changed = true;
    }

    const ClangDiagnosticConfigs oldConfigs = m_settings->clangCustomDiagnosticConfigs();
    const ClangDiagnosticConfigs currentConfigs = m_ui->clangDiagnosticConfigsSelectionWidget
                                                      ->customConfigs();
    if (oldConfigs != currentConfigs) {
        m_settings->setClangCustomDiagnosticConfigs(currentConfigs);
        changed = true;
    }

    return changed;
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

CppCodeModelSettingsPage::CppCodeModelSettingsPage(CppCodeModelSettings *settings)
{
    setId(Constants::CPP_CODE_MODEL_SETTINGS_ID);
    setDisplayName(CppCodeModelSettingsWidget::tr("Code Model"));
    setCategory(Constants::CPP_SETTINGS_CATEGORY);
    setDisplayCategory(QCoreApplication::translate("CppEditor", "C++"));
    setCategoryIconPath(":/projectexplorer/images/settingscategory_cpp.png");
    setWidgetCreator([settings] { return new CppCodeModelSettingsWidget(settings); });
}

class ClangdSettingsWidget::Private
{
public:
    QCheckBox useClangdCheckBox;
    QCheckBox indexingCheckBox;
    QCheckBox autoIncludeHeadersCheckBox;
    QSpinBox threadLimitSpinBox;
    QSpinBox documentUpdateThreshold;
    Utils::PathChooser clangdChooser;
    Utils::InfoLabel versionWarningLabel;
};

ClangdSettingsWidget::ClangdSettingsWidget(const ClangdSettings::Data &settingsData)
    : d(new Private)
{
    const ClangdSettings settings(settingsData);
    d->useClangdCheckBox.setText(tr("Use clangd (EXPERIMENTAL)"));
    d->useClangdCheckBox.setChecked(settings.useClangd());
    d->clangdChooser.setExpectedKind(Utils::PathChooser::ExistingCommand);
    d->clangdChooser.setFilePath(settings.clangdFilePath());
    d->clangdChooser.setEnabled(d->useClangdCheckBox.isChecked());
    d->indexingCheckBox.setChecked(settings.indexingEnabled());
    d->indexingCheckBox.setToolTip(tr(
        "If background indexing is enabled, global symbol searches will yield\n"
        "more accurate results, at the cost of additional CPU load when\n"
        "the project is first opened."));
    d->autoIncludeHeadersCheckBox.setChecked(settings.autoIncludeHeaders());
    d->autoIncludeHeadersCheckBox.setToolTip(tr(
        "Controls whether clangd may insert header files as part of symbol completion."));
    d->threadLimitSpinBox.setValue(settings.workerThreadLimit());
    d->threadLimitSpinBox.setSpecialValueText("Automatic");
    d->documentUpdateThreshold.setMinimum(50);
    d->documentUpdateThreshold.setMaximum(10000);
    d->documentUpdateThreshold.setValue(settings.documentUpdateThreshold());
    d->documentUpdateThreshold.setSingleStep(100);
    d->documentUpdateThreshold.setSuffix(" ms");
    d->documentUpdateThreshold.setToolTip(
        tr("Defines the amount of time Qt Creator waits before sending document changes to the "
           "server.\n"
           "If the document changes again while waiting, this timeout resets.\n"));

    const auto layout = new QVBoxLayout(this);
    layout->addWidget(&d->useClangdCheckBox);
    const auto formLayout = new QFormLayout;
    const auto chooserLabel = new QLabel(tr("Path to executable:"));
    formLayout->addRow(chooserLabel, &d->clangdChooser);
    formLayout->addRow(QString(), &d->versionWarningLabel);
    const auto indexingLabel = new QLabel(tr("Enable background indexing:"));
    formLayout->addRow(indexingLabel, &d->indexingCheckBox);
    const auto autoIncludeHeadersLabel = new QLabel(tr("Insert header files on completion:"));
    formLayout->addRow(autoIncludeHeadersLabel, &d->autoIncludeHeadersCheckBox);
    const auto threadLimitLayout = new QHBoxLayout;
    threadLimitLayout->addWidget(&d->threadLimitSpinBox);
    threadLimitLayout->addStretch(1);
    const auto threadLimitLabel = new QLabel(tr("Worker thread count:"));
    formLayout->addRow(threadLimitLabel, threadLimitLayout);
    const auto documentUpdateThresholdLayout = new QHBoxLayout;
    documentUpdateThresholdLayout->addWidget(&d->documentUpdateThreshold);
    documentUpdateThresholdLayout->addStretch(1);
    const auto documentUpdateThresholdLabel = new QLabel(tr("Document update threshold:"));
    formLayout->addRow(documentUpdateThresholdLabel, documentUpdateThresholdLayout);
    layout->addLayout(formLayout);
    layout->addStretch(1);

    static const auto setWidgetsEnabled = [](QLayout *layout, bool enabled, const auto &f) -> void {
        for (int i = 0; i < layout->count(); ++i) {
            if (QWidget * const w = layout->itemAt(i)->widget())
                w->setEnabled(enabled);
            else if (QLayout * const l = layout->itemAt(i)->layout())
                f(l, enabled, f);
        }
    };
    const auto toggleEnabled = [formLayout](const bool checked) {
        setWidgetsEnabled(formLayout, checked, setWidgetsEnabled);
    };
    connect(&d->useClangdCheckBox, &QCheckBox::toggled, toggleEnabled);
    toggleEnabled(d->useClangdCheckBox.isChecked());
    d->threadLimitSpinBox.setEnabled(d->useClangdCheckBox.isChecked());

    d->versionWarningLabel.setType(Utils::InfoLabel::Warning);
    const auto updateWarningLabel = [this] {
        class WarningLabelSetter {
        public:
            WarningLabelSetter(QLabel &label) : m_label(label) { m_label.clear(); }
            ~WarningLabelSetter() { m_label.setVisible(!m_label.text().isEmpty()); }
            void setWarning(const QString &text) { m_label.setText(text); }
        private:
            QLabel &m_label;
        };
        WarningLabelSetter labelSetter(d->versionWarningLabel);

        if (!d->clangdChooser.isValid())
            return;
        const Utils::FilePath clangdPath = d->clangdChooser.filePath();
        const QVersionNumber clangdVersion = ClangdSettings::clangdVersion(clangdPath);
        if (clangdVersion.isNull()) {
            labelSetter.setWarning(tr("Failed to retrieve clangd version: "
                                      "Unexpected clangd output."));
            return;
        }
        if (clangdVersion < QVersionNumber(13)) {
            labelSetter.setWarning(tr("The clangd version is %1, but %2 or greater is required.")
                                   .arg(clangdVersion.toString()).arg(13));
            return;
        }
    };
    connect(&d->clangdChooser, &Utils::PathChooser::pathChanged, this, updateWarningLabel);
    updateWarningLabel();

    connect(&d->useClangdCheckBox, &QCheckBox::toggled,
            this, &ClangdSettingsWidget::settingsDataChanged);
    connect(&d->indexingCheckBox, &QCheckBox::toggled,
            this, &ClangdSettingsWidget::settingsDataChanged);
    connect(&d->autoIncludeHeadersCheckBox, &QCheckBox::toggled,
            this, &ClangdSettingsWidget::settingsDataChanged);
    connect(&d->threadLimitSpinBox, qOverload<int>(&QSpinBox::valueChanged),
            this, &ClangdSettingsWidget::settingsDataChanged);
    connect(&d->clangdChooser, &Utils::PathChooser::pathChanged,
            this, &ClangdSettingsWidget::settingsDataChanged);
}

ClangdSettingsWidget::~ClangdSettingsWidget()
{
    delete d;
}

ClangdSettings::Data ClangdSettingsWidget::settingsData() const
{
    ClangdSettings::Data data;
    data.useClangd = d->useClangdCheckBox.isChecked();
    data.executableFilePath = d->clangdChooser.filePath();
    data.enableIndexing = d->indexingCheckBox.isChecked();
    data.autoIncludeHeaders = d->autoIncludeHeadersCheckBox.isChecked();
    data.workerThreadLimit = d->threadLimitSpinBox.value();
    data.documentUpdateThreshold = d->documentUpdateThreshold.value();
    return data;
}

class ClangdSettingsPageWidget final : public Core::IOptionsPageWidget
{
    Q_DECLARE_TR_FUNCTIONS(CppEditor::Internal::ClangdSettingsWidget)

public:
    ClangdSettingsPageWidget() : m_widget(ClangdSettings::instance().data())
    {
        const auto layout = new QVBoxLayout(this);
        layout->addWidget(&m_widget);
    }

private:
    void apply() final { ClangdSettings::instance().setData(m_widget.settingsData()); }

    ClangdSettingsWidget m_widget;
};

ClangdSettingsPage::ClangdSettingsPage()
{
    setId("K.Clangd");
    setDisplayName(ClangdSettingsWidget::tr("Clangd"));
    setCategory(Constants::CPP_SETTINGS_CATEGORY);
    setWidgetCreator([] { return new ClangdSettingsPageWidget; });
}


class ClangdProjectSettingsWidget::Private
{
public:
    Private(const ClangdProjectSettings &s) : settings(s), widget(s.settings()) {}

    ClangdProjectSettings settings;
    ClangdSettingsWidget widget;
    QCheckBox useGlobalSettingsCheckBox;
};

ClangdProjectSettingsWidget::ClangdProjectSettingsWidget(const ClangdProjectSettings &settings)
    : d(new Private(settings))
{
    const auto layout = new QVBoxLayout(this);
    d->useGlobalSettingsCheckBox.setText(tr("Use global settings"));
    layout->addWidget(&d->useGlobalSettingsCheckBox);
    const auto separator = new QFrame;
    separator->setFrameShape(QFrame::HLine);
    layout->addWidget(separator);
    layout->addWidget(&d->widget);

    d->useGlobalSettingsCheckBox.setChecked(d->settings.useGlobalSettings());
    d->widget.setEnabled(!d->settings.useGlobalSettings());
    connect(&d->useGlobalSettingsCheckBox, &QCheckBox::toggled, [this](bool checked) {
        d->widget.setEnabled(!checked);
        d->settings.setUseGlobalSettings(checked);
        if (!checked)
            d->settings.setSettings(d->widget.settingsData());
    });
    connect(&d->widget, &ClangdSettingsWidget::settingsDataChanged, [this] {
        d->settings.setSettings(d->widget.settingsData());
    });
}

ClangdProjectSettingsWidget::~ClangdProjectSettingsWidget()
{
    delete d;
}

} // CppEditor::Internal
