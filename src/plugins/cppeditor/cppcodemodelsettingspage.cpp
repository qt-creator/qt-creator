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

#include "clangdiagnosticconfigsselectionwidget.h"
#include "clangdiagnosticconfigswidget.h"
#include "cppeditorconstants.h"
#include "cpptoolsreuse.h"

#include <coreplugin/icore.h>
#include <projectexplorer/session.h>
#include <utils/algorithm.h>
#include <utils/infolabel.h>
#include <utils/itemviews.h>
#include <utils/pathchooser.h>
#include <utils/qtcassert.h>

#include <QDesktopServices>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QPushButton>
#include <QSpinBox>
#include <QStringListModel>
#include <QTextStream>
#include <QVBoxLayout>
#include <QVersionNumber>

#include <limits>

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

    bool applyGeneralWidgetsToSettings() const;

    Ui::CppCodeModelSettingsPage *m_ui = nullptr;
    CppCodeModelSettings *m_settings = nullptr;
};

CppCodeModelSettingsWidget::CppCodeModelSettingsWidget(CppCodeModelSettings *s)
    : m_ui(new Ui::CppCodeModelSettingsPage)
{
    m_ui->setupUi(this);

    m_settings = s;

    setupGeneralWidgets();
}

CppCodeModelSettingsWidget::~CppCodeModelSettingsWidget()
{
    delete m_ui;
}

void CppCodeModelSettingsWidget::apply()
{
    if (applyGeneralWidgetsToSettings())
        m_settings->toSettings(Core::ICore::settings());
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
    QCheckBox sizeThresholdCheckBox;
    QSpinBox threadLimitSpinBox;
    QSpinBox documentUpdateThreshold;
    QSpinBox sizeThresholdSpinBox;
    Utils::PathChooser clangdChooser;
    Utils::InfoLabel versionWarningLabel;
    ClangDiagnosticConfigsSelectionWidget *configSelectionWidget = nullptr;
    QGroupBox *sessionsGroupBox = nullptr;
    QStringListModel sessionsModel;
};

ClangdSettingsWidget::ClangdSettingsWidget(const ClangdSettings::Data &settingsData,
                                           bool isForProject)
    : d(new Private)
{
    const ClangdSettings settings(settingsData);
    d->useClangdCheckBox.setText(tr("Use clangd"));
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
    d->sizeThresholdCheckBox.setText(tr("Ignore files greater than"));
    d->sizeThresholdCheckBox.setChecked(settings.sizeThresholdEnabled());
    d->sizeThresholdSpinBox.setMinimum(1);
    d->sizeThresholdSpinBox.setMaximum(std::numeric_limits<int>::max());
    d->sizeThresholdSpinBox.setSuffix(" KB");
    d->sizeThresholdSpinBox.setValue(settings.sizeThresholdInKb());
    d->sizeThresholdSpinBox.setToolTip(tr(
            "Files greater than this will not be opened as documents in clangd.\n"
            "The built-in code model will handle highlighting, completion and so on."));

    const auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
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
    const auto sizeThresholdLayout = new QHBoxLayout;
    sizeThresholdLayout->addWidget(&d->sizeThresholdSpinBox);
    sizeThresholdLayout->addStretch(1);
    formLayout->addRow(&d->sizeThresholdCheckBox, sizeThresholdLayout);
    d->configSelectionWidget = new ClangDiagnosticConfigsSelectionWidget(formLayout);
    d->configSelectionWidget->refresh(
                diagnosticConfigsModel(settings.customDiagnosticConfigs()),
                settings.diagnosticConfigId(),
                [](const ClangDiagnosticConfigs &configs, const Utils::Id &configToSelect) {
                    return new CppEditor::ClangDiagnosticConfigsWidget(configs, configToSelect);
                });

    layout->addLayout(formLayout);
    if (!isForProject) {
        d->sessionsModel.setStringList(settingsData.sessionsWithOneClangd);
        d->sessionsModel.sort(0);
        d->sessionsGroupBox = new QGroupBox(tr("Sessions with a single clangd instance"));
        const auto sessionsView = new Utils::ListView;
        sessionsView->setModel(&d->sessionsModel);
        sessionsView->setToolTip(
                    tr("By default, Qt Creator runs one clangd process per project.\n"
                       "If you have sessions with tightly coupled projects that should be\n"
                       "managed by the same clangd process, add them here."));
        const auto outerSessionsLayout = new QHBoxLayout;
        const auto innerSessionsLayout = new QHBoxLayout(d->sessionsGroupBox);
        const auto buttonsLayout = new QVBoxLayout;
        const auto addButton = new QPushButton(tr("Add ..."));
        const auto removeButton = new QPushButton(tr("Remove"));
        buttonsLayout->addWidget(addButton);
        buttonsLayout->addWidget(removeButton);
        buttonsLayout->addStretch(1);
        innerSessionsLayout->addWidget(sessionsView);
        innerSessionsLayout->addLayout(buttonsLayout);
        outerSessionsLayout->addWidget(d->sessionsGroupBox);
        outerSessionsLayout->addStretch(1);
        layout->addLayout(outerSessionsLayout);

        const auto updateRemoveButtonState = [removeButton, sessionsView] {
            removeButton->setEnabled(sessionsView->selectionModel()->hasSelection());
        };
        connect(sessionsView->selectionModel(), &QItemSelectionModel::selectionChanged,
                this, updateRemoveButtonState);
        updateRemoveButtonState();
        connect(removeButton, &QPushButton::clicked, this, [this, sessionsView] {
            const QItemSelection selection = sessionsView->selectionModel()->selection();
            QTC_ASSERT(!selection.isEmpty(), return);
            d->sessionsModel.removeRow(selection.indexes().first().row());
        });

        connect(addButton, &QPushButton::clicked, this, [this, sessionsView] {
            QInputDialog dlg(sessionsView);
            QStringList sessions = ProjectExplorer::SessionManager::sessions();
            QStringList currentSessions = d->sessionsModel.stringList();
            for (const QString &s : qAsConst(currentSessions))
                sessions.removeOne(s);
            if (sessions.isEmpty())
                return;
            sessions.sort();
            dlg.setLabelText(tr("Choose a session:"));
            dlg.setComboBoxItems(sessions);
            if (dlg.exec() == QDialog::Accepted) {
                currentSessions << dlg.textValue();
                d->sessionsModel.setStringList(currentSessions);
                d->sessionsModel.sort(0);
            }
        });

        // TODO: Remove once the concept is functional.
        d->sessionsGroupBox->hide();
    }

    const auto configFilesHelpLabel = new QLabel;
    configFilesHelpLabel->setText(tr("Additional settings are available via "
            "<a href=\"https://clangd.llvm.org/config\"> clangd configuration files</a>.<br>"
            "General settings go <a href=\"%1\">here</a> "
            "and can be overridden per project by putting a .clangd file into "
            "the project source tree.")
                .arg(ClangdSettings::clangdUserConfigFilePath().toUserOutput()));
    connect(configFilesHelpLabel, &QLabel::linkHovered, configFilesHelpLabel, &QLabel::setToolTip);
    connect(configFilesHelpLabel, &QLabel::linkActivated, [](const QString &link) {
        if (link.startsWith("https"))
            QDesktopServices::openUrl(link);
        else
            Core::EditorManager::openEditor(Utils::FilePath::fromString(link));
    });
    const auto separator = new QFrame;
    separator->setFrameShape(QFrame::HLine);
    layout->addWidget(separator);
    layout->addWidget(configFilesHelpLabel);

    layout->addStretch(1);

    static const auto setWidgetsEnabled = [](QLayout *layout, bool enabled, const auto &f) -> void {
        for (int i = 0; i < layout->count(); ++i) {
            if (QWidget * const w = layout->itemAt(i)->widget())
                w->setEnabled(enabled);
            else if (QLayout * const l = layout->itemAt(i)->layout())
                f(l, enabled, f);
        }
    };
    const auto toggleEnabled = [this, formLayout](const bool checked) {
        setWidgetsEnabled(formLayout, checked, setWidgetsEnabled);
        if (d->sessionsGroupBox)
            d->sessionsGroupBox->setEnabled(checked);
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
        if (clangdVersion < QVersionNumber(14)) {
            labelSetter.setWarning(tr("The clangd version is %1, but %2 or greater is required.")
                                   .arg(clangdVersion.toString()).arg(14));
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
    connect(&d->sizeThresholdCheckBox, &QCheckBox::toggled,
            this, &ClangdSettingsWidget::settingsDataChanged);
    connect(&d->sizeThresholdSpinBox, qOverload<int>(&QSpinBox::valueChanged),
            this, &ClangdSettingsWidget::settingsDataChanged);
    connect(&d->documentUpdateThreshold, qOverload<int>(&QSpinBox::valueChanged),
            this, &ClangdSettingsWidget::settingsDataChanged);
    connect(&d->clangdChooser, &Utils::PathChooser::pathChanged,
            this, &ClangdSettingsWidget::settingsDataChanged);
    connect(d->configSelectionWidget, &ClangDiagnosticConfigsSelectionWidget::changed,
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
    data.sizeThresholdEnabled = d->sizeThresholdCheckBox.isChecked();
    data.sizeThresholdInKb = d->sizeThresholdSpinBox.value();
    data.sessionsWithOneClangd = d->sessionsModel.stringList();
    data.customDiagnosticConfigs = d->configSelectionWidget->customConfigs();
    data.diagnosticConfigId = d->configSelectionWidget->currentConfigId();
    return data;
}

class ClangdSettingsPageWidget final : public Core::IOptionsPageWidget
{
    Q_DECLARE_TR_FUNCTIONS(CppEditor::Internal::ClangdSettingsWidget)

public:
    ClangdSettingsPageWidget() : m_widget(ClangdSettings::instance().data(), false)
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
    setId(Constants::CPP_CLANGD_SETTINGS_ID);
    setDisplayName(ClangdSettingsWidget::tr("Clangd"));
    setCategory(Constants::CPP_SETTINGS_CATEGORY);
    setWidgetCreator([] { return new ClangdSettingsPageWidget; });
}


class ClangdProjectSettingsWidget::Private
{
public:
    Private(const ClangdProjectSettings &s) : settings(s), widget(s.settings(), true) {}

    ClangdProjectSettings settings;
    ClangdSettingsWidget widget;
    QCheckBox useGlobalSettingsCheckBox;
};

ClangdProjectSettingsWidget::ClangdProjectSettingsWidget(const ClangdProjectSettings &settings)
    : d(new Private(settings))
{
    const auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    const auto globalSettingsLayout = new QHBoxLayout;
    globalSettingsLayout->addWidget(&d->useGlobalSettingsCheckBox);
    const auto globalSettingsLabel = new QLabel("Use <a href=\"dummy\">global settings</a>");
    connect(globalSettingsLabel, &QLabel::linkActivated,
            this, [] { Core::ICore::showOptionsDialog(Constants::CPP_CLANGD_SETTINGS_ID); });
    globalSettingsLayout->addWidget(globalSettingsLabel);
    globalSettingsLayout->addStretch(1);
    layout->addLayout(globalSettingsLayout);

    const auto separator = new QFrame;
    separator->setFrameShape(QFrame::HLine);
    layout->addWidget(separator);
    layout->addWidget(&d->widget);

    const auto updateGlobalSettingsCheckBox = [this] {
        if (ClangdSettings::instance().granularity() == ClangdSettings::Granularity::Session) {
            d->useGlobalSettingsCheckBox.setEnabled(false);
            d->useGlobalSettingsCheckBox.setChecked(true);
        } else {
            d->useGlobalSettingsCheckBox.setEnabled(true);
            d->useGlobalSettingsCheckBox.setChecked(d->settings.useGlobalSettings());
        }
        d->widget.setEnabled(!d->useGlobalSettingsCheckBox.isChecked());
    };
    updateGlobalSettingsCheckBox();
    connect(&ClangdSettings::instance(), &ClangdSettings::changed,
            this, updateGlobalSettingsCheckBox);

    connect(&d->useGlobalSettingsCheckBox, &QCheckBox::clicked, [this](bool checked) {
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
