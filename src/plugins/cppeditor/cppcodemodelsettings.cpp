// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cppcodemodelsettings.h"

#include "compileroptionsbuilder.h"
#include "cppeditorconstants.h"
#include "cppeditortr.h"
#include "cppmodelmanager.h"

#include <coreplugin/dialogs/ioptionspage.h>
#include <coreplugin/icore.h>

#include <projectexplorer/project.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/projectpanelfactory.h>
#include <projectexplorer/projectsettingswidget.h>

#include <utils/algorithm.h>
#include <utils/hostosinfo.h>
#include <utils/layoutbuilder.h>
#include <utils/macroexpander.h>
#include <utils/qtcassert.h>
#include <utils/qtcsettings.h>
#include <utils/store.h>

#include <QCheckBox>
#include <QDateTime>
#include <QDir>
#include <QHash>
#include <QPair>
#include <QPlainTextEdit>
#include <QSettings>
#include <QSpinBox>
#include <QTimer>
#include <QVBoxLayout>

using namespace ProjectExplorer;
using namespace Utils;

namespace CppEditor {

static Key pchUsageKey() { return Constants::CPPEDITOR_MODEL_MANAGER_PCH_USAGE; }
static Key interpretAmbiguousHeadersAsCHeadersKey()
    { return Constants::CPPEDITOR_INTERPRET_AMBIGIUOUS_HEADERS_AS_C_HEADERS; }
static Key enableIndexingKey() { return "EnableIndexing"; }
static Key skipIndexingBigFilesKey() { return Constants::CPPEDITOR_SKIP_INDEXING_BIG_FILES; }
static Key ignoreFilesKey() { return Constants::CPPEDITOR_IGNORE_FILES; }
static Key ignorePatternKey() { return Constants::CPPEDITOR_IGNORE_PATTERN; }
static Key useBuiltinPreprocessorKey() { return Constants::CPPEDITOR_USE_BUILTIN_PREPROCESSOR; }
static Key indexerFileSizeLimitKey() { return Constants::CPPEDITOR_INDEXER_FILE_SIZE_LIMIT; }
static Key useGlobalSettingsKey() { return "useGlobalSettings"; }

CppCodeModelSettings::CppCodeModelSettings()
{
}

bool operator==(const CppEditor::CppCodeModelSettingsData &s1,
                const CppEditor::CppCodeModelSettingsData &s2)
{
    return s1.pchUsage == s2.pchUsage
        && s1.interpretAmbiguousHeadersAsC == s2.interpretAmbiguousHeadersAsC
        && s1.enableIndexing == s2.enableIndexing
        && s1.skipIndexingBigFiles == s2.skipIndexingBigFiles
        && s1.useBuiltinPreprocessor == s2.useBuiltinPreprocessor
        && s1.indexerFileSizeLimitInMb == s2.indexerFileSizeLimitInMb
        && s1.ignoreFiles == s2.ignoreFiles
        && s1.ignorePattern == s2.ignorePattern;
}

Store CppCodeModelSettingsData::toMap() const
{
    Store store;
    store.insert(pchUsageKey(), int(pchUsage));
    store.insert(interpretAmbiguousHeadersAsCHeadersKey(), interpretAmbiguousHeadersAsC);
    store.insert(enableIndexingKey(), enableIndexing);
    store.insert(skipIndexingBigFilesKey(), skipIndexingBigFiles);
    store.insert(ignoreFilesKey(), ignoreFiles);
    store.insert(ignorePatternKey(), ignorePattern);
    store.insert(useBuiltinPreprocessorKey(), useBuiltinPreprocessor);
    store.insert(indexerFileSizeLimitKey(), indexerFileSizeLimitInMb);
    return store;
}

void CppCodeModelSettingsData::fromMap(const Utils::Store &store)
{
    const CppCodeModelSettingsData def;
    pchUsage = static_cast<PchUsage>(store.value(pchUsageKey(), int(def.pchUsage)).toInt());
    interpretAmbiguousHeadersAsC
        = store.value(interpretAmbiguousHeadersAsCHeadersKey(), def.interpretAmbiguousHeadersAsC)
              .toBool();
    enableIndexing = store.value(enableIndexingKey(), def.enableIndexing).toBool();
    skipIndexingBigFiles
        = store.value(skipIndexingBigFilesKey(), def.skipIndexingBigFiles).toBool();
    ignoreFiles = store.value(ignoreFilesKey(), def.ignoreFiles).toBool();
    ignorePattern = store.value(ignorePatternKey(), def.ignorePattern).toString();
    useBuiltinPreprocessor
        = store.value(useBuiltinPreprocessorKey(), def.useBuiltinPreprocessor).toBool();
    indexerFileSizeLimitInMb
        = store.value(indexerFileSizeLimitKey(), def.indexerFileSizeLimitInMb).toInt();
}

void CppCodeModelSettingsData::fromSettings(QtcSettings *s)
{
    fromMap(storeFromSettings(Constants::CPPEDITOR_SETTINGSGROUP, s));
}

void CppCodeModelSettingsData::toSettings(QtcSettings *s) const
{
    storeToSettingsWithDefault(
        Constants::CPPEDITOR_SETTINGSGROUP, s, toMap(), CppCodeModelSettingsData().toMap());
}

CppCodeModelSettings &CppCodeModelSettings::globalInstance()
{
    static CppCodeModelSettings theCppCodeModelSettings(Core::ICore::settings());
    return theCppCodeModelSettings;
}

CppCodeModelSettingsData CppCodeModelSettings::settingsForProject(const Utils::FilePath &projectFile)
{
    return settingsForProject(ProjectManager::projectWithProjectFilePath(projectFile));
}

CppCodeModelSettingsData CppCodeModelSettings::settingsForFile(const Utils::FilePath &file)
{
    return settingsForProject(ProjectManager::projectForFile(file));
}

void CppCodeModelSettings::setGlobal(const CppCodeModelSettingsData &settings)
{
    if (globalInstance().data() == settings)
        return;

    globalInstance().setData(settings);
    globalInstance().data().toSettings(Core::ICore::settings());
    CppModelManager::handleSettingsChange(nullptr);
}

PchUsage CppCodeModelSettings::pchUsageForProject(const Project *project)
{
    return CppCodeModelSettings::settingsForProject(project).pchUsage;
}

UsePrecompiledHeaders CppCodeModelSettingsData::usePrecompiledHeaders() const
{
    return pchUsage == PchUsage::PchUse_None ? UsePrecompiledHeaders::No
                                             : UsePrecompiledHeaders::Yes;
}

UsePrecompiledHeaders CppCodeModelSettings::usePrecompiledHeaders(const Project *project)
{
    return CppCodeModelSettings::settingsForProject(project).usePrecompiledHeaders();
}

int CppCodeModelSettingsData::effectiveIndexerFileSizeLimitInMb() const
{
    return skipIndexingBigFiles ? indexerFileSizeLimitInMb : -1;
}

bool CppCodeModelSettings::categorizeFindReferences()
{
    return globalInstance().m_categorizeFindReferences;
}

void CppCodeModelSettings::setCategorizeFindReferences(bool categorize)
{
    globalInstance().m_categorizeFindReferences = categorize;
}

bool CppCodeModelSettings::isInteractiveFollowSymbol()
{
    return globalInstance().interactiveFollowSymbol;
}

void CppCodeModelSettings::setInteractiveFollowSymbol(bool interactive)
{
    globalInstance().interactiveFollowSymbol = interactive;
}

namespace Internal {

class CppCodeModelSettingsWidget final : public Core::IOptionsPageWidget
{
    Q_OBJECT
public:
    CppCodeModelSettingsWidget() = default;

    void setup(const CppCodeModelSettingsData &settings);
    CppCodeModelSettingsData settings() const;

signals:
    void settingsDataChanged();

private:
    void apply() final { CppCodeModelSettings::setGlobal(settings()); }

    QCheckBox *m_interpretAmbiguousHeadersAsCHeaders;
    QCheckBox *m_ignorePchCheckBox;
    QCheckBox *m_useBuiltinPreprocessorCheckBox;
    QCheckBox *m_enableIndexingCheckBox;
    QCheckBox *m_skipIndexingBigFilesCheckBox;
    QSpinBox *m_bigFilesLimitSpinBox;
    QCheckBox *m_ignoreFilesCheckBox;
    QPlainTextEdit *m_ignorePatternTextEdit;
};

void CppCodeModelSettingsWidget::setup(const CppCodeModelSettingsData &settings)
{
    m_interpretAmbiguousHeadersAsCHeaders
        = new QCheckBox(Tr::tr("Interpret ambiguous headers as C headers"));

    m_enableIndexingCheckBox = new QCheckBox(Tr::tr("Enable indexing"));
    m_enableIndexingCheckBox->setChecked(settings.enableIndexing);
    m_enableIndexingCheckBox->setToolTip(Tr::tr(
        "Indexing should almost always be kept enabled, as disabling it will severely limit the "
        "capabilities of the code model."));

    m_skipIndexingBigFilesCheckBox = new QCheckBox(Tr::tr("Do not index files greater than"));
    m_skipIndexingBigFilesCheckBox->setChecked(settings.skipIndexingBigFiles);

    m_bigFilesLimitSpinBox = new QSpinBox;
    m_bigFilesLimitSpinBox->setSuffix(Tr::tr("MB"));
    m_bigFilesLimitSpinBox->setRange(1, 500);
    m_bigFilesLimitSpinBox->setValue(settings.indexerFileSizeLimitInMb);

    m_ignoreFilesCheckBox = new QCheckBox(Tr::tr("Ignore files"));
    m_ignoreFilesCheckBox->setToolTip(
        "<html><head/><body><p>"
        + Tr::tr("Ignore files that match these wildcard patterns, one wildcard per line.")
        + "</p></body></html>");

    m_ignoreFilesCheckBox->setChecked(settings.ignoreFiles);
    m_ignorePatternTextEdit = new QPlainTextEdit(settings.ignorePattern);
    m_ignorePatternTextEdit->setToolTip(m_ignoreFilesCheckBox->toolTip());
    m_ignorePatternTextEdit->setEnabled(m_ignoreFilesCheckBox->isChecked());

    connect(m_ignoreFilesCheckBox, &QCheckBox::stateChanged, this, [this] {
        m_ignorePatternTextEdit->setEnabled(m_ignoreFilesCheckBox->isChecked());
    });
    connect(m_ignorePatternTextEdit, &QPlainTextEdit::textChanged, this, markSettingsDirty);

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

    m_interpretAmbiguousHeadersAsCHeaders->setChecked(settings.interpretAmbiguousHeadersAsC);
    m_ignorePchCheckBox->setChecked(settings.pchUsage == PchUsage::PchUse_None);
    m_useBuiltinPreprocessorCheckBox->setChecked(settings.useBuiltinPreprocessor);

    using namespace Layouting;

    Column {
        Group {
            title(Tr::tr("General")),
            Column {
                   m_interpretAmbiguousHeadersAsCHeaders,
                   m_ignorePchCheckBox,
                   m_useBuiltinPreprocessorCheckBox,
                   m_enableIndexingCheckBox,
                   Row { m_skipIndexingBigFilesCheckBox, m_bigFilesLimitSpinBox, st },
                   Row { Column { m_ignoreFilesCheckBox, st }, m_ignorePatternTextEdit },
                   }
        },
        st
    }.attachTo(this);

    for (const QCheckBox *const b : {m_interpretAmbiguousHeadersAsCHeaders,
                                     m_ignorePchCheckBox,
                                     m_enableIndexingCheckBox,
                                     m_useBuiltinPreprocessorCheckBox,
                                     m_skipIndexingBigFilesCheckBox,
                                     m_ignoreFilesCheckBox}) {
        connect(b, &QCheckBox::toggled, this, &CppCodeModelSettingsWidget::settingsDataChanged);
    }
    connect(m_bigFilesLimitSpinBox, &QSpinBox::valueChanged,
            this, &CppCodeModelSettingsWidget::settingsDataChanged);

    const auto timer = new QTimer(this);
    timer->setSingleShot(true);
    timer->setInterval(1000);
    connect(timer, &QTimer::timeout, this, &CppCodeModelSettingsWidget::settingsDataChanged);
    connect(m_ignorePatternTextEdit, &QPlainTextEdit::textChanged,
            timer, qOverload<>(&QTimer::start));

    installMarkSettingsDirtyTriggerRecursively(this);
}

CppCodeModelSettingsData CppCodeModelSettingsWidget::settings() const
{
    CppCodeModelSettingsData settings;
    settings.interpretAmbiguousHeadersAsC = m_interpretAmbiguousHeadersAsCHeaders->isChecked();
    settings.enableIndexing = m_enableIndexingCheckBox->isChecked();
    settings.skipIndexingBigFiles = m_skipIndexingBigFilesCheckBox->isChecked();
    settings.useBuiltinPreprocessor = m_useBuiltinPreprocessorCheckBox->isChecked();
    settings.ignoreFiles = m_ignoreFilesCheckBox->isChecked();
    settings.ignorePattern = m_ignorePatternTextEdit->toPlainText();
    settings.indexerFileSizeLimitInMb = m_bigFilesLimitSpinBox->value();
    settings.pchUsage = m_ignorePchCheckBox->isChecked() ? PchUsage::PchUse_None
                                                         : PchUsage::PchUse_BuildSystem;
    return settings;
}

class CppCodeModelSettingsPage final : public Core::IOptionsPage
{
public:
    CppCodeModelSettingsPage()
    {
        setId(Constants::CPP_CODE_MODEL_SETTINGS_ID);
        setDisplayName(Tr::tr("Code Model"));
        setCategory(Constants::CPP_SETTINGS_CATEGORY);
        setWidgetCreator([] {
            auto w = new CppCodeModelSettingsWidget;
            w->setup(CppCodeModelSettings::global().data());
            return w;
        });
    }
};

void setupCppCodeModelSettingsPage()
{
    static CppCodeModelSettingsPage theCppCodeModelSettingsPage;
}

class CppCodeModelProjectSettingsWidget : public ProjectSettingsWidget
{
public:
    explicit CppCodeModelProjectSettingsWidget(Project *project)
        : m_project(project)
    {
        if (project) {
            const Store data = storeFromVariant(m_project->namedSettings(Constants::CPPEDITOR_SETTINGSGROUP));
            m_useGlobalSettings = data.value(useGlobalSettingsKey(), true).toBool();
            CppCodeModelSettingsData d;
            d.fromMap(data);
            m_customSettings.setData(d);
        }

        m_widget.setup(m_useGlobalSettings ? CppCodeModelSettings::global().data() : m_customSettings.data());

        setGlobalSettingsId(Constants::CPP_CODE_MODEL_SETTINGS_ID);
        const auto layout = new QVBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->addWidget(&m_widget);

        setUseGlobalSettings(m_useGlobalSettings);
        m_widget.setEnabled(!useGlobalSettings());
        connect(this, &ProjectSettingsWidget::useGlobalSettingsChanged, this,
                [this](bool checked) {
                    m_widget.setEnabled(!checked);
                    m_useGlobalSettings = checked;
                    if (!checked)
                        m_customSettings.setData(m_widget.settings());
                    saveSettings();
                });

        connect(&m_widget, &CppCodeModelSettingsWidget::settingsDataChanged, this, [this] {
            m_customSettings.setData(m_widget.settings());
            saveSettings();
        });
    }

    void saveSettings()
    {
        if (m_project) {
            Store data = m_customSettings.data().toMap();
            data.insert(useGlobalSettingsKey(), m_useGlobalSettings);
            m_project->setNamedSettings(Constants::CPPEDITOR_SETTINGSGROUP, variantFromStore(data));
        }
        CppModelManager::handleSettingsChange(m_project);
    }

private:
    Project * const m_project;
    CppCodeModelSettings m_customSettings;
    bool m_useGlobalSettings = true;
    CppCodeModelSettingsWidget m_widget;
};

} // namespace Internal

bool CppCodeModelSettings::hasCustomSettings(const Project *project)
{
    if (!project)
        return false;
    CppCodeModelSettings m_customSettings;
    const Store data = storeFromVariant(project->namedSettings(Constants::CPPEDITOR_SETTINGSGROUP));
    return !data.value(useGlobalSettingsKey(), true).toBool();
}

void CppCodeModelSettings::setSettingsForProject(Project *project, const CppCodeModelSettingsData &settings)
{
    if (project) {
        Store data = settings.toMap();
        data.insert(useGlobalSettingsKey(), false);
        project->setNamedSettings(Constants::CPPEDITOR_SETTINGSGROUP, variantFromStore(data));
    }
    CppModelManager::handleSettingsChange(project);
}

CppCodeModelSettingsData CppCodeModelSettings::settingsForProject(const Project *project)
{
    if (!project)
        return CppCodeModelSettings::globalInstance().data();

    const Store data = storeFromVariant(project->namedSettings(Constants::CPPEDITOR_SETTINGSGROUP));
    if (data.value(useGlobalSettingsKey(), true).toBool())
        return CppCodeModelSettings::globalInstance().data();

    CppCodeModelSettingsData customSettings;
    customSettings.fromMap(data);
    return customSettings;
}

namespace Internal {

class CppCodeModelProjectSettingsPanelFactory final : public ProjectPanelFactory
{
public:
    CppCodeModelProjectSettingsPanelFactory()
    {
        setPriority(100);
        setDisplayName(Tr::tr("C++ Code Model"));
        setCreateWidgetFunction([](Project *project) {
            return new CppCodeModelProjectSettingsWidget(project);
        });
    }
};

void setupCppCodeModelProjectSettingsPanel()
{
    static CppCodeModelProjectSettingsPanelFactory factory;
}

} // namespace Internal
} // namespace CppEditor

#include <cppcodemodelsettings.moc>
