// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cppcodemodelsettingspage.h"

#include "clangdsettings.h"
#include "cppcodemodelsettings.h"
#include "cppeditorconstants.h"
#include "cppeditortr.h"

#include <coreplugin/dialogs/ioptionspage.h>
#include <coreplugin/icore.h>
#include <coreplugin/session.h>

#include <projectexplorer/projectpanelfactory.h>
#include <projectexplorer/projectsettingswidget.h>

#include <utils/algorithm.h>
#include <utils/clangutils.h>
#include <utils/fancylineedit.h>
#include <utils/infolabel.h>
#include <utils/layoutbuilder.h>
#include <utils/pathchooser.h>
#include <utils/qtcassert.h>

#include <QCheckBox>
#include <QComboBox>
#include <QPlainTextEdit>
#include <QSpinBox>
#include <QTextBlock>
#include <QTextStream>
#include <QTimer>
#include <QVBoxLayout>
#include <QVersionNumber>

using namespace ProjectExplorer;

namespace CppEditor::Internal {

class CppCodeModelSettingsWidget final : public Core::IOptionsPageWidget
{
    Q_OBJECT
public:
    CppCodeModelSettingsWidget(const CppCodeModelSettings::Data &data);

    CppCodeModelSettings::Data data() const;

signals:
    void settingsDataChanged();

private:
    void apply() final { CppCodeModelSettings::globalInstance().setGlobalData(data()); }

    QCheckBox *m_interpretAmbiguousHeadersAsCHeaders;
    QCheckBox *m_ignorePchCheckBox;
    QCheckBox *m_useBuiltinPreprocessorCheckBox;
    QCheckBox *m_skipIndexingBigFilesCheckBox;
    QSpinBox *m_bigFilesLimitSpinBox;
    QCheckBox *m_ignoreFilesCheckBox;
    QPlainTextEdit *m_ignorePatternTextEdit;
};

CppCodeModelSettingsWidget::CppCodeModelSettingsWidget(const CppCodeModelSettings::Data &data)
{
    m_interpretAmbiguousHeadersAsCHeaders
        = new QCheckBox(Tr::tr("Interpret ambiguous headers as C headers"));

    m_skipIndexingBigFilesCheckBox = new QCheckBox(Tr::tr("Do not index files greater than"));
    m_skipIndexingBigFilesCheckBox->setChecked(data.skipIndexingBigFiles);

    m_bigFilesLimitSpinBox = new QSpinBox;
    m_bigFilesLimitSpinBox->setSuffix(Tr::tr("MB"));
    m_bigFilesLimitSpinBox->setRange(1, 500);
    m_bigFilesLimitSpinBox->setValue(data.indexerFileSizeLimitInMb);

    m_ignoreFilesCheckBox = new QCheckBox(Tr::tr("Ignore files"));
    m_ignoreFilesCheckBox->setToolTip(
        "<html><head/><body><p>"
        + Tr::tr("Ignore files that match these wildcard patterns, one wildcard per line.")
        + "</p></body></html>");

    m_ignoreFilesCheckBox->setChecked(data.ignoreFiles);
    m_ignorePatternTextEdit = new QPlainTextEdit(data.ignorePattern);
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

    m_interpretAmbiguousHeadersAsCHeaders->setChecked(data.interpretAmbigiousHeadersAsC);
    m_ignorePchCheckBox->setChecked(data.pchUsage == CppCodeModelSettings::PchUse_None);
    m_useBuiltinPreprocessorCheckBox->setChecked(data.useBuiltinPreprocessor);

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

    for (const QCheckBox *const b : {m_interpretAmbiguousHeadersAsCHeaders,
                                     m_ignorePchCheckBox,
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
}

CppCodeModelSettings::Data CppCodeModelSettingsWidget::data() const
{
    CppCodeModelSettings::Data data;
    data.interpretAmbigiousHeadersAsC = m_interpretAmbiguousHeadersAsCHeaders->isChecked();
    data.skipIndexingBigFiles = m_skipIndexingBigFilesCheckBox->isChecked();
    data.useBuiltinPreprocessor = m_useBuiltinPreprocessorCheckBox->isChecked();
    data.ignoreFiles = m_ignoreFilesCheckBox->isChecked();
    data.ignorePattern = m_ignorePatternTextEdit->toPlainText();
    data.indexerFileSizeLimitInMb = m_bigFilesLimitSpinBox->value();
    data.pchUsage = m_ignorePchCheckBox->isChecked() ? CppCodeModelSettings::PchUse_None
                                                     : CppCodeModelSettings::PchUse_BuildSystem;
    return data;
}

class CppCodeModelSettingsPage final : public Core::IOptionsPage
{
public:
    CppCodeModelSettingsPage()
    {
        setId(Constants::CPP_CODE_MODEL_SETTINGS_ID);
        setDisplayName(Tr::tr("Code Model"));
        setCategory(Constants::CPP_SETTINGS_CATEGORY);
        setDisplayCategory(Tr::tr("C++"));
        setCategoryIconPath(":/projectexplorer/images/settingscategory_cpp.png");
        setWidgetCreator(
            [] { return new CppCodeModelSettingsWidget(CppCodeModelSettings::globalInstance().data()); });
    }
};

void setupCppCodeModelSettingsPage()
{
    static CppCodeModelSettingsPage theCppCodeModelSettingsPage;
}

class CppCodeModelProjectSettingsWidget : public ProjectSettingsWidget
{
public:
    CppCodeModelProjectSettingsWidget(const CppCodeModelProjectSettings &settings)
        : m_settings(settings), m_widget(settings.data())
    {
        setGlobalSettingsId(Constants::CPP_CODE_MODEL_SETTINGS_ID);
        const auto layout = new QVBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->addWidget(&m_widget);

        setUseGlobalSettings(m_settings.useGlobalSettings());
        m_widget.setEnabled(!useGlobalSettings());
        connect(this, &ProjectSettingsWidget::useGlobalSettingsChanged, this,
                [this](bool checked) {
                    m_widget.setEnabled(!checked);
                    m_settings.setUseGlobalSettings(checked);
                    if (!checked)
                        m_settings.setData(m_widget.data());
                });

        connect(&m_widget, &CppCodeModelSettingsWidget::settingsDataChanged,
                this, [this] { m_settings.setData(m_widget.data()); });
    }

private:
    CppCodeModelProjectSettings m_settings;
    CppCodeModelSettingsWidget m_widget;
};

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

} // CppEditor::Internal

#include "cppcodemodelsettingspage.moc"
