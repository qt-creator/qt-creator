// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cppcodemodelsettingspage.h"

#include "clangdiagnosticconfigsselectionwidget.h"
#include "clangdiagnosticconfigswidget.h"
#include "cppeditorconstants.h"
#include "cppeditortr.h"
#include "cpptoolsreuse.h"

#include <coreplugin/icore.h>
#include <coreplugin/session.h>

#include <utils/algorithm.h>
#include <utils/infolabel.h>
#include <utils/itemviews.h>
#include <utils/layoutbuilder.h>
#include <utils/pathchooser.h>
#include <utils/qtcassert.h>

#include <QCheckBox>
#include <QComboBox>
#include <QDesktopServices>
#include <QFormLayout>
#include <QGroupBox>
#include <QGuiApplication>
#include <QInputDialog>
#include <QPushButton>
#include <QSpinBox>
#include <QStringListModel>
#include <QTextBlock>
#include <QTextStream>
#include <QVBoxLayout>
#include <QVersionNumber>

#include <limits>

namespace CppEditor::Internal {

class CppCodeModelSettingsWidget final : public Core::IOptionsPageWidget
{
public:
    CppCodeModelSettingsWidget(CppCodeModelSettings *s);

private:
    void apply() final;

    bool applyGeneralWidgetsToSettings() const;

    CppCodeModelSettings *m_settings = nullptr;
    QCheckBox *m_interpretAmbiguousHeadersAsCHeaders;
    QCheckBox *m_ignorePchCheckBox;
    QCheckBox *m_useBuiltinPreprocessorCheckBox;
    QCheckBox *m_skipIndexingBigFilesCheckBox;
    QSpinBox *m_bigFilesLimitSpinBox;
    QCheckBox *m_ignoreFilesCheckBox;
    QPlainTextEdit *m_ignorePatternTextEdit;
};

CppCodeModelSettingsWidget::CppCodeModelSettingsWidget(CppCodeModelSettings *s)
    : m_settings(s)
{
    m_interpretAmbiguousHeadersAsCHeaders
        = new QCheckBox(Tr::tr("Interpret ambiguous headers as C headers"));

    m_skipIndexingBigFilesCheckBox = new QCheckBox(Tr::tr("Do not index files greater than"));
    m_skipIndexingBigFilesCheckBox->setChecked(m_settings->skipIndexingBigFiles());

    m_bigFilesLimitSpinBox = new QSpinBox;
    m_bigFilesLimitSpinBox->setSuffix(Tr::tr("MB"));
    m_bigFilesLimitSpinBox->setRange(1, 500);
    m_bigFilesLimitSpinBox->setValue(m_settings->indexerFileSizeLimitInMb());

    m_ignoreFilesCheckBox = new QCheckBox(Tr::tr("Ignore files"));
    m_ignoreFilesCheckBox->setToolTip(
        "<html><head/><body><p>"
        + Tr::tr("Ignore files that match these wildcard patterns, one wildcard per line.")
        + "</p></body></html>");

    m_ignoreFilesCheckBox->setChecked(m_settings->ignoreFiles());
    m_ignorePatternTextEdit = new QPlainTextEdit(m_settings->ignorePattern());
    m_ignorePatternTextEdit->setToolTip(m_ignoreFilesCheckBox->toolTip());
    m_ignorePatternTextEdit->setEnabled(m_ignoreFilesCheckBox->isChecked());

    connect(m_ignoreFilesCheckBox, &QCheckBox::stateChanged, this, [this] {
       m_ignorePatternTextEdit->setEnabled(m_ignoreFilesCheckBox->isChecked());
    });

    m_ignorePchCheckBox = new QCheckBox(Tr::tr("Ignore precompiled headers"));
    m_ignorePchCheckBox->setToolTip(Tr::tr(
        "<html><head/><body><p>When precompiled headers are not ignored, the parsing for code "
        "completion and semantic highlighting will process the precompiled header before "
        "processing any file.</p></body></html>"));

    m_useBuiltinPreprocessorCheckBox = new QCheckBox(Tr::tr("Use built-in preprocessor to show "
                                                            "pre-processed files"));
    m_useBuiltinPreprocessorCheckBox->setToolTip
            (Tr::tr("Uncheck this to invoke the actual compiler "
                    "to show a pre-processed source file in the editor."));

    m_interpretAmbiguousHeadersAsCHeaders->setChecked(
                m_settings->interpretAmbigiousHeadersAsCHeaders());

    m_ignorePchCheckBox->setChecked(m_settings->pchUsage() == CppCodeModelSettings::PchUse_None);
    m_useBuiltinPreprocessorCheckBox->setChecked(m_settings->useBuiltinPreprocessor());

    using namespace Layouting;

    Column {
        Group {
            title(Tr::tr("General")),
            Column {
                m_interpretAmbiguousHeadersAsCHeaders,
                m_ignorePchCheckBox,
                m_useBuiltinPreprocessorCheckBox,
                Row { m_skipIndexingBigFilesCheckBox, m_bigFilesLimitSpinBox, st },
                Row { Column { m_ignoreFilesCheckBox, st }, m_ignorePatternTextEdit },
            }
        },
        st
    }.attachTo(this);
}

void CppCodeModelSettingsWidget::apply()
{
    if (applyGeneralWidgetsToSettings())
        m_settings->toSettings(Core::ICore::settings());
}

bool CppCodeModelSettingsWidget::applyGeneralWidgetsToSettings() const
{
    bool settingsChanged = false;

    const bool newInterpretAmbiguousHeaderAsCHeaders
            = m_interpretAmbiguousHeadersAsCHeaders->isChecked();
    if (m_settings->interpretAmbigiousHeadersAsCHeaders()
            != newInterpretAmbiguousHeaderAsCHeaders) {
        m_settings->setInterpretAmbigiousHeadersAsCHeaders(newInterpretAmbiguousHeaderAsCHeaders);
        settingsChanged = true;
    }

    const bool newSkipIndexingBigFiles = m_skipIndexingBigFilesCheckBox->isChecked();
    if (m_settings->skipIndexingBigFiles() != newSkipIndexingBigFiles) {
        m_settings->setSkipIndexingBigFiles(newSkipIndexingBigFiles);
        settingsChanged = true;
    }
    const bool newUseBuiltinPreprocessor = m_useBuiltinPreprocessorCheckBox->isChecked();
    if (m_settings->useBuiltinPreprocessor() != newUseBuiltinPreprocessor) {
        m_settings->setUseBuiltinPreprocessor(newUseBuiltinPreprocessor);
        settingsChanged = true;
    }
    const bool ignoreFiles = m_ignoreFilesCheckBox->isChecked();
    if (m_settings->ignoreFiles() != ignoreFiles) {
        m_settings->setIgnoreFiles(ignoreFiles);
        settingsChanged = true;
    }
    const QString ignorePattern = m_ignorePatternTextEdit->toPlainText();
    if (m_settings->ignorePattern() != ignorePattern) {
        m_settings->setIgnorePattern(ignorePattern);
        settingsChanged = true;
    }
    const int newFileSizeLimit = m_bigFilesLimitSpinBox->value();
    if (m_settings->indexerFileSizeLimitInMb() != newFileSizeLimit) {
        m_settings->setIndexerFileSizeLimitInMb(newFileSizeLimit);
        settingsChanged = true;
    }

    const bool newIgnorePch = m_ignorePchCheckBox->isChecked();
    const bool previousIgnorePch = m_settings->pchUsage() == CppCodeModelSettings::PchUse_None;
    if (newIgnorePch != previousIgnorePch) {
        const CppCodeModelSettings::PCHUsage pchUsage = m_ignorePchCheckBox->isChecked()
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
    setDisplayName(Tr::tr("Code Model"));
    setCategory(Constants::CPP_SETTINGS_CATEGORY);
    setDisplayCategory(Tr::tr("C++"));
    setCategoryIconPath(":/projectexplorer/images/settingscategory_cpp.png");
    setWidgetCreator([settings] { return new CppCodeModelSettingsWidget(settings); });
}

class ClangdSettingsWidget::Private
{
public:
    QCheckBox useClangdCheckBox;
    QComboBox indexingComboBox;
    QComboBox headerSourceSwitchComboBox;
    QComboBox completionRankingModelComboBox;
    QCheckBox autoIncludeHeadersCheckBox;
    QCheckBox sizeThresholdCheckBox;
    QSpinBox threadLimitSpinBox;
    QSpinBox documentUpdateThreshold;
    QSpinBox sizeThresholdSpinBox;
    QSpinBox completionResults;
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
    const QString indexingToolTip = Tr::tr(
        "<p>If background indexing is enabled, global symbol searches will yield more accurate "
        "results, at the cost of additional CPU load when the project is first opened. The "
        "indexing result is persisted in the project's build directory. If you disable background "
        "indexing, a faster, but less accurate, built-in indexer is used instead. The thread "
        "priority for building the background index can be adjusted since clangd 15.</p>"
        "<p>Background Priority: Minimum priority, runs on idle CPUs. May leave 'performance' "
        "cores unused.</p>"
        "<p>Normal Priority: Reduced priority compared to interactive work.</p>"
        "<p>Low Priority: Same priority as other clangd work.</p>");
    const QString headerSourceSwitchToolTip = Tr::tr(
        "<p>The C/C++ backend to use for switching between header and source files.</p>"
        "<p>While the clangd implementation has more capabilities than the built-in "
        "code model, it tends to find false positives.</p>"
        "<p>When \"Try Both\" is selected, clangd is used only if the built-in variant "
        "does not find anything.</p>");
    using RankingModel = ClangdSettings::CompletionRankingModel;
    const QString completionRankingModelToolTip = Tr::tr(
        "<p>Which model clangd should use to rank possible completions.</p>"
        "<p>This determines the order of candidates in the combo box when doing code completion.</p>"
        "<p>The \"%1\" model used by default results from (pre-trained) machine learning and "
        "provides superior results on average.</p>"
        "<p>If you feel that its suggestions stray too much from your expectations for your "
        "code base, you can try switching to the hand-crafted \"%2\" model.</p>").arg(
            ClangdSettings::rankingModelToDisplayString(RankingModel::DecisionForest),
            ClangdSettings::rankingModelToDisplayString(RankingModel::Heuristics));
    const QString workerThreadsToolTip = Tr::tr(
        "Number of worker threads used by clangd. Background indexing also uses this many "
        "worker threads.");
    const QString autoIncludeToolTip = Tr::tr(
        "Controls whether clangd may insert header files as part of symbol completion.");
    const QString documentUpdateToolTip
        //: %1 is the application name (Qt Creator)
        = Tr::tr("Defines the amount of time %1 waits before sending document changes to the "
                 "server.\n"
                 "If the document changes again while waiting, this timeout resets.")
              .arg(QGuiApplication::applicationDisplayName());
    const QString sizeThresholdToolTip = Tr::tr(
        "Files greater than this will not be opened as documents in clangd.\n"
        "The built-in code model will handle highlighting, completion and so on.");
    const QString completionResultToolTip = Tr::tr(
        "The maximum number of completion results returned by clangd.");

    d->useClangdCheckBox.setText(Tr::tr("Use clangd"));
    d->useClangdCheckBox.setChecked(settings.useClangd());
    d->clangdChooser.setExpectedKind(Utils::PathChooser::ExistingCommand);
    d->clangdChooser.setFilePath(settings.clangdFilePath());
    d->clangdChooser.setAllowPathFromDevice(true);
    d->clangdChooser.setEnabled(d->useClangdCheckBox.isChecked());
    d->clangdChooser.setCommandVersionArguments({"--version"});
    using Priority = ClangdSettings::IndexingPriority;
    for (Priority prio : {Priority::Off, Priority::Background, Priority::Low, Priority::Normal}) {
        d->indexingComboBox.addItem(ClangdSettings::priorityToDisplayString(prio), int(prio));
        if (prio == settings.indexingPriority())
            d->indexingComboBox.setCurrentIndex(d->indexingComboBox.count() - 1);
    }
    d->indexingComboBox.setToolTip(indexingToolTip);
    using SwitchMode = ClangdSettings::HeaderSourceSwitchMode;
    for (SwitchMode mode : {SwitchMode::BuiltinOnly, SwitchMode::ClangdOnly, SwitchMode::Both}) {
        d->headerSourceSwitchComboBox.addItem(
            ClangdSettings::headerSourceSwitchModeToDisplayString(mode), int(mode));
        if (mode == settings.headerSourceSwitchMode())
            d->headerSourceSwitchComboBox.setCurrentIndex(
                d->headerSourceSwitchComboBox.count() - 1);
    }
    d->headerSourceSwitchComboBox.setToolTip(headerSourceSwitchToolTip);
    for (RankingModel model : {RankingModel::Default, RankingModel::DecisionForest,
                               RankingModel::Heuristics}) {
        d->completionRankingModelComboBox.addItem(
            ClangdSettings::rankingModelToDisplayString(model), int(model));
        if (model == settings.completionRankingModel())
            d->completionRankingModelComboBox.setCurrentIndex(
                d->completionRankingModelComboBox.count() - 1);
    }
    d->completionRankingModelComboBox.setToolTip(completionRankingModelToolTip);

    d->autoIncludeHeadersCheckBox.setText(Tr::tr("Insert header files on completion"));
    d->autoIncludeHeadersCheckBox.setChecked(settings.autoIncludeHeaders());
    d->autoIncludeHeadersCheckBox.setToolTip(autoIncludeToolTip);
    d->threadLimitSpinBox.setValue(settings.workerThreadLimit());
    d->threadLimitSpinBox.setSpecialValueText(Tr::tr("Automatic"));
    d->threadLimitSpinBox.setToolTip(workerThreadsToolTip);
    d->documentUpdateThreshold.setMinimum(50);
    d->documentUpdateThreshold.setMaximum(10000);
    d->documentUpdateThreshold.setValue(settings.documentUpdateThreshold());
    d->documentUpdateThreshold.setSingleStep(100);
    d->documentUpdateThreshold.setSuffix(" ms");
    d->documentUpdateThreshold.setToolTip(documentUpdateToolTip);
    d->sizeThresholdCheckBox.setText(Tr::tr("Ignore files greater than"));
    d->sizeThresholdCheckBox.setChecked(settings.sizeThresholdEnabled());
    d->sizeThresholdCheckBox.setToolTip(sizeThresholdToolTip);
    d->sizeThresholdSpinBox.setMinimum(1);
    d->sizeThresholdSpinBox.setMaximum(std::numeric_limits<int>::max());
    d->sizeThresholdSpinBox.setSuffix(" KB");
    d->sizeThresholdSpinBox.setValue(settings.sizeThresholdInKb());
    d->sizeThresholdSpinBox.setToolTip(sizeThresholdToolTip);

    const auto completionResultsLabel = new QLabel(Tr::tr("Completion results:"));
    completionResultsLabel->setToolTip(completionResultToolTip);
    d->completionResults.setMinimum(0);
    d->completionResults.setMaximum(std::numeric_limits<int>::max());
    d->completionResults.setValue(settings.completionResults());
    d->completionResults.setToolTip(completionResultToolTip);
    d->completionResults.setSpecialValueText(Tr::tr("No limit"));

    const auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(&d->useClangdCheckBox);

    const auto formLayout = new QFormLayout;
    const auto chooserLabel = new QLabel(Tr::tr("Path to executable:"));
    formLayout->addRow(chooserLabel, &d->clangdChooser);
    formLayout->addRow(QString(), &d->versionWarningLabel);

    const auto indexingPriorityLayout = new QHBoxLayout;
    indexingPriorityLayout->addWidget(&d->indexingComboBox);
    indexingPriorityLayout->addStretch(1);
    const auto indexingPriorityLabel = new QLabel(Tr::tr("Background indexing:"));
    indexingPriorityLabel->setToolTip(indexingToolTip);
    formLayout->addRow(indexingPriorityLabel, indexingPriorityLayout);

    const auto headerSourceSwitchLayout = new QHBoxLayout;
    headerSourceSwitchLayout->addWidget(&d->headerSourceSwitchComboBox);
    headerSourceSwitchLayout->addStretch(1);
    const auto headerSourceSwitchLabel = new QLabel(Tr::tr("Header/source switch mode:"));
    headerSourceSwitchLabel->setToolTip(headerSourceSwitchToolTip);
    formLayout->addRow(headerSourceSwitchLabel, headerSourceSwitchLayout);

    const auto threadLimitLayout = new QHBoxLayout;
    threadLimitLayout->addWidget(&d->threadLimitSpinBox);
    threadLimitLayout->addStretch(1);
    const auto threadLimitLabel = new QLabel(Tr::tr("Worker thread count:"));
    threadLimitLabel->setToolTip(workerThreadsToolTip);
    formLayout->addRow(threadLimitLabel, threadLimitLayout);

    formLayout->addRow(QString(), &d->autoIncludeHeadersCheckBox);
    const auto limitResultsLayout = new QHBoxLayout;
    limitResultsLayout->addWidget(&d->completionResults);
    limitResultsLayout->addStretch(1);
    formLayout->addRow(completionResultsLabel, limitResultsLayout);

    const auto completionRankingModelLayout = new QHBoxLayout;
    completionRankingModelLayout->addWidget(&d->completionRankingModelComboBox);
    completionRankingModelLayout->addStretch(1);
    const auto completionRankingModelLabel = new QLabel(Tr::tr("Completion ranking model:"));
    completionRankingModelLabel->setToolTip(completionRankingModelToolTip);
    formLayout->addRow(completionRankingModelLabel, completionRankingModelLayout);

    const auto documentUpdateThresholdLayout = new QHBoxLayout;
    documentUpdateThresholdLayout->addWidget(&d->documentUpdateThreshold);
    documentUpdateThresholdLayout->addStretch(1);
    const auto documentUpdateThresholdLabel = new QLabel(Tr::tr("Document update threshold:"));
    documentUpdateThresholdLabel->setToolTip(documentUpdateToolTip);
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
        d->sessionsGroupBox = new QGroupBox(Tr::tr("Sessions with a single clangd instance"));
        const auto sessionsView = new Utils::ListView;
        sessionsView->setModel(&d->sessionsModel);
        sessionsView->setToolTip(
                    Tr::tr("By default, Qt Creator runs one clangd process per project.\n"
                       "If you have sessions with tightly coupled projects that should be\n"
                       "managed by the same clangd process, add them here."));
        const auto outerSessionsLayout = new QHBoxLayout;
        const auto innerSessionsLayout = new QHBoxLayout(d->sessionsGroupBox);
        const auto buttonsLayout = new QVBoxLayout;
        const auto addButton = new QPushButton(Tr::tr("Add ..."));
        const auto removeButton = new QPushButton(Tr::tr("Remove"));
        buttonsLayout->addWidget(addButton);
        buttonsLayout->addWidget(removeButton);
        buttonsLayout->addStretch(1);
        innerSessionsLayout->addWidget(sessionsView);
        innerSessionsLayout->addLayout(buttonsLayout);
        outerSessionsLayout->addWidget(d->sessionsGroupBox);
        outerSessionsLayout->addStretch(1);

        const auto separator = new QFrame;
        separator->setFrameShape(QFrame::HLine);
        layout->addWidget(separator);
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
            QStringList sessions = Core::SessionManager::sessions();
            QStringList currentSessions = d->sessionsModel.stringList();
            for (const QString &s : std::as_const(currentSessions))
                sessions.removeOne(s);
            if (sessions.isEmpty())
                return;
            sessions.sort();
            dlg.setLabelText(Tr::tr("Choose a session:"));
            dlg.setComboBoxItems(sessions);
            if (dlg.exec() == QDialog::Accepted) {
                currentSessions << dlg.textValue();
                d->sessionsModel.setStringList(currentSessions);
                d->sessionsModel.sort(0);
            }
        });
    }

    const auto configFilesHelpLabel = new QLabel;
    configFilesHelpLabel->setText(Tr::tr("Additional settings are available via "
            "<a href=\"https://clangd.llvm.org/config\"> clangd configuration files</a>.<br>"
            "User-specific settings go <a href=\"%1\">here</a>, "
            "project-specific settings can be configured by putting a .clangd file into "
            "the project source tree.")
                .arg(ClangdSettings::clangdUserConfigFilePath().toUserOutput()));
    configFilesHelpLabel->setWordWrap(true);
    connect(configFilesHelpLabel, &QLabel::linkHovered, configFilesHelpLabel, &QLabel::setToolTip);
    connect(configFilesHelpLabel, &QLabel::linkActivated, [](const QString &link) {
        if (link.startsWith("https"))
            QDesktopServices::openUrl(link);
        else
            Core::EditorManager::openEditor(Utils::FilePath::fromString(link));
    });
    layout->addWidget(Layouting::createHr());
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
        QString errorMessage;
        if (!Utils::checkClangdVersion(clangdPath, &errorMessage))
            labelSetter.setWarning(errorMessage);
    };
    connect(&d->clangdChooser, &Utils::PathChooser::textChanged, this, updateWarningLabel);
    connect(&d->clangdChooser, &Utils::PathChooser::validChanged, this, updateWarningLabel);
    updateWarningLabel();

    connect(&d->useClangdCheckBox, &QCheckBox::toggled,
            this, &ClangdSettingsWidget::settingsDataChanged);
    connect(&d->indexingComboBox, &QComboBox::currentIndexChanged,
            this, &ClangdSettingsWidget::settingsDataChanged);
    connect(&d->headerSourceSwitchComboBox, &QComboBox::currentIndexChanged,
            this, &ClangdSettingsWidget::settingsDataChanged);
    connect(&d->completionRankingModelComboBox, &QComboBox::currentIndexChanged,
            this, &ClangdSettingsWidget::settingsDataChanged);
    connect(&d->autoIncludeHeadersCheckBox, &QCheckBox::toggled,
            this, &ClangdSettingsWidget::settingsDataChanged);
    connect(&d->threadLimitSpinBox, &QSpinBox::valueChanged,
            this, &ClangdSettingsWidget::settingsDataChanged);
    connect(&d->sizeThresholdCheckBox, &QCheckBox::toggled,
            this, &ClangdSettingsWidget::settingsDataChanged);
    connect(&d->sizeThresholdSpinBox, &QSpinBox::valueChanged,
            this, &ClangdSettingsWidget::settingsDataChanged);
    connect(&d->documentUpdateThreshold, &QSpinBox::valueChanged,
            this, &ClangdSettingsWidget::settingsDataChanged);
    connect(&d->clangdChooser, &Utils::PathChooser::textChanged,
            this, &ClangdSettingsWidget::settingsDataChanged);
    connect(d->configSelectionWidget, &ClangDiagnosticConfigsSelectionWidget::changed,
            this, &ClangdSettingsWidget::settingsDataChanged);
    connect(&d->completionResults, &QSpinBox::valueChanged,
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
    data.indexingPriority = ClangdSettings::IndexingPriority(
        d->indexingComboBox.currentData().toInt());
    data.headerSourceSwitchMode = ClangdSettings::HeaderSourceSwitchMode(
        d->headerSourceSwitchComboBox.currentData().toInt());
    data.completionRankingModel = ClangdSettings::CompletionRankingModel(
        d->completionRankingModelComboBox.currentData().toInt());
    data.autoIncludeHeaders = d->autoIncludeHeadersCheckBox.isChecked();
    data.workerThreadLimit = d->threadLimitSpinBox.value();
    data.documentUpdateThreshold = d->documentUpdateThreshold.value();
    data.sizeThresholdEnabled = d->sizeThresholdCheckBox.isChecked();
    data.sizeThresholdInKb = d->sizeThresholdSpinBox.value();
    data.sessionsWithOneClangd = d->sessionsModel.stringList();
    data.customDiagnosticConfigs = d->configSelectionWidget->customConfigs();
    data.diagnosticConfigId = d->configSelectionWidget->currentConfigId();
    data.completionResults = d->completionResults.value();
    return data;
}

class ClangdSettingsPageWidget final : public Core::IOptionsPageWidget
{
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
    setDisplayName(Tr::tr("Clangd"));
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
    setGlobalSettingsId(Constants::CPP_CLANGD_SETTINGS_ID);
    const auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(&d->widget);

    const auto updateGlobalSettingsCheckBox = [this] {
        if (ClangdSettings::instance().granularity() == ClangdSettings::Granularity::Session) {
            setUseGlobalSettingsCheckBoxEnabled(false);
            setUseGlobalSettings(true);
        } else {
            setUseGlobalSettingsCheckBoxEnabled(true);
            setUseGlobalSettings(d->settings.useGlobalSettings());
        }
        d->widget.setEnabled(!useGlobalSettings());
    };

    updateGlobalSettingsCheckBox();
    connect(&ClangdSettings::instance(), &ClangdSettings::changed,
            this, updateGlobalSettingsCheckBox);

    connect(this, &ProjectSettingsWidget::useGlobalSettingsChanged, this,
            [this](bool checked) {
                d->widget.setEnabled(!checked);
                d->settings.setUseGlobalSettings(checked);
                if (!checked)
                    d->settings.setSettings(d->widget.settingsData());
            });

    connect(&d->widget, &ClangdSettingsWidget::settingsDataChanged, this, [this] {
        d->settings.setSettings(d->widget.settingsData());
    });
}

ClangdProjectSettingsWidget::~ClangdProjectSettingsWidget()
{
    delete d;
}

} // CppEditor::Internal
