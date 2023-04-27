// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmljseditingsettingspage.h"
#include "qmljseditorconstants.h"
#include "qmljseditortr.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/icore.h>
#include <utils/layoutbuilder.h>
#include <utils/macroexpander.h>
#include <utils/qtcsettings.h>
#include <utils/variablechooser.h>
#include <qmljstools/qmljstoolsconstants.h>

#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QSettings>
#include <QTextStream>

const char AUTO_FORMAT_ON_SAVE[] = "QmlJSEditor.AutoFormatOnSave";
const char AUTO_FORMAT_ONLY_CURRENT_PROJECT[] = "QmlJSEditor.AutoFormatOnlyCurrentProject";
const char QML_CONTEXTPANE_KEY[] = "QmlJSEditor.ContextPaneEnabled";
const char QML_CONTEXTPANEPIN_KEY[] = "QmlJSEditor.ContextPanePinned";
const char FOLD_AUX_DATA[] = "QmlJSEditor.FoldAuxData";
const char USE_QMLLS[] = "QmlJSEditor.UseQmlls";
const char USE_LATEST_QMLLS[] = "QmlJSEditor.UseLatestQmlls";
const char UIQML_OPEN_MODE[] = "QmlJSEditor.openUiQmlMode";
const char FORMAT_COMMAND[] = "QmlJSEditor.formatCommand";
const char FORMAT_COMMAND_OPTIONS[] = "QmlJSEditor.formatCommandOptions";
const char CUSTOM_COMMAND[] = "QmlJSEditor.useCustomFormatCommand";
const char DEFAULT_CUSTOM_FORMAT_COMMAND[] = "%{CurrentDocument:Project:QT_HOST_BINS}/qmlformat";

using namespace QmlJSEditor;
using namespace QmlJSEditor::Internal;

void QmlJsEditingSettings::set()
{
    if (get() != *this)
        toSettings(Core::ICore::settings());
}

void QmlJsEditingSettings::fromSettings(QSettings *settings)
{
    settings->beginGroup(QmlJSEditor::Constants::SETTINGS_CATEGORY_QML);
    m_enableContextPane = settings->value(QML_CONTEXTPANE_KEY, QVariant(false)).toBool();
    m_pinContextPane = settings->value(QML_CONTEXTPANEPIN_KEY, QVariant(false)).toBool();
    m_autoFormatOnSave = settings->value(AUTO_FORMAT_ON_SAVE, QVariant(false)).toBool();
    m_autoFormatOnlyCurrentProject
        = settings->value(AUTO_FORMAT_ONLY_CURRENT_PROJECT, QVariant(false)).toBool();
    m_foldAuxData = settings->value(FOLD_AUX_DATA, QVariant(true)).toBool();
    m_uiQmlOpenMode = settings->value(UIQML_OPEN_MODE, "").toString();
    m_qmllsSettings.useQmlls = settings->value(USE_QMLLS, QVariant(false)).toBool();
    m_qmllsSettings.useLatestQmlls = settings->value(USE_LATEST_QMLLS, QVariant(false)).toBool();
    m_formatCommand = settings->value(FORMAT_COMMAND, {}).toString();
    m_formatCommandOptions = settings->value(FORMAT_COMMAND_OPTIONS, {}).toString();
    m_useCustomFormatCommand = settings->value(CUSTOM_COMMAND, QVariant(false)).toBool();
    settings->endGroup();
}

void QmlJsEditingSettings::toSettings(QSettings *settings) const
{
    settings->beginGroup(QmlJSEditor::Constants::SETTINGS_CATEGORY_QML);
    settings->setValue(QML_CONTEXTPANE_KEY, m_enableContextPane);
    settings->setValue(QML_CONTEXTPANEPIN_KEY, m_pinContextPane);
    settings->setValue(AUTO_FORMAT_ON_SAVE, m_autoFormatOnSave);
    settings->setValue(AUTO_FORMAT_ONLY_CURRENT_PROJECT, m_autoFormatOnlyCurrentProject);
    settings->setValue(FOLD_AUX_DATA, m_foldAuxData);
    settings->setValue(UIQML_OPEN_MODE, m_uiQmlOpenMode);
    settings->setValue(USE_QMLLS, m_qmllsSettings.useQmlls);
    settings->setValue(USE_LATEST_QMLLS, m_qmllsSettings.useLatestQmlls);
    Utils::QtcSettings::setValueWithDefault(settings, FORMAT_COMMAND, m_formatCommand, {});
    Utils::QtcSettings::setValueWithDefault(settings,
                                            FORMAT_COMMAND_OPTIONS,
                                            m_formatCommandOptions,
                                            {});
    Utils::QtcSettings::setValueWithDefault(settings,
                                            CUSTOM_COMMAND,
                                            m_useCustomFormatCommand,
                                            false);
    settings->endGroup();
    QmllsSettingsManager::instance()->checkForChanges();
}

bool QmlJsEditingSettings::equals(const QmlJsEditingSettings &other) const
{
    return m_enableContextPane == other.m_enableContextPane
           && m_pinContextPane == other.m_pinContextPane
           && m_autoFormatOnSave == other.m_autoFormatOnSave
           && m_autoFormatOnlyCurrentProject == other.m_autoFormatOnlyCurrentProject
           && m_foldAuxData == other.m_foldAuxData && m_qmllsSettings == other.m_qmllsSettings
           && m_uiQmlOpenMode == other.m_uiQmlOpenMode && m_formatCommand == other.m_formatCommand
           && m_formatCommandOptions == other.m_formatCommandOptions
           && m_useCustomFormatCommand == other.m_useCustomFormatCommand;
}

bool QmlJsEditingSettings::enableContextPane() const
{
    return m_enableContextPane;
}

void QmlJsEditingSettings::setEnableContextPane(const bool enableContextPane)
{
    m_enableContextPane = enableContextPane;
}

bool QmlJsEditingSettings::pinContextPane() const
{
    return m_pinContextPane;
}

void QmlJsEditingSettings::setPinContextPane(const bool pinContextPane)
{
    m_pinContextPane = pinContextPane;
}

bool QmlJsEditingSettings::autoFormatOnSave() const
{
    return m_autoFormatOnSave;
}

void QmlJsEditingSettings::setAutoFormatOnSave(const bool autoFormatOnSave)
{
    m_autoFormatOnSave = autoFormatOnSave;
}

bool QmlJsEditingSettings::autoFormatOnlyCurrentProject() const
{
    return m_autoFormatOnlyCurrentProject;
}

void QmlJsEditingSettings::setAutoFormatOnlyCurrentProject(const bool autoFormatOnlyCurrentProject)
{
    m_autoFormatOnlyCurrentProject = autoFormatOnlyCurrentProject;
}

bool QmlJsEditingSettings::foldAuxData() const
{
    return m_foldAuxData;
}

void QmlJsEditingSettings::setFoldAuxData(const bool foldAuxData)
{
    m_foldAuxData = foldAuxData;
}

QString QmlJsEditingSettings::defaultFormatCommand() const
{
    return DEFAULT_CUSTOM_FORMAT_COMMAND;
}

QString QmlJsEditingSettings::formatCommand() const
{
    return m_formatCommand;
}

void QmlJsEditingSettings::setFormatCommand(const QString &formatCommand)
{
    m_formatCommand = formatCommand;
}

QString QmlJsEditingSettings::formatCommandOptions() const
{
    return m_formatCommandOptions;
}

void QmlJsEditingSettings::setFormatCommandOptions(const QString &formatCommandOptions)
{
    m_formatCommandOptions = formatCommandOptions;
}

bool QmlJsEditingSettings::useCustomFormatCommand() const
{
    return m_useCustomFormatCommand;
}

void QmlJsEditingSettings::setUseCustomFormatCommand(bool customCommand)
{
    m_useCustomFormatCommand = customCommand;
}

QmllsSettings &QmlJsEditingSettings::qmllsSettings()
{
    return m_qmllsSettings;
}

const QmllsSettings &QmlJsEditingSettings::qmllsSettings() const
{
    return m_qmllsSettings;
}

const QString QmlJsEditingSettings::uiQmlOpenMode() const
{
    return m_uiQmlOpenMode;
}

void QmlJsEditingSettings::setUiQmlOpenMode(const QString &mode)
{
    m_uiQmlOpenMode = mode;
}

class QmlJsEditingSettingsPageWidget final : public Core::IOptionsPageWidget
{
public:
    QmlJsEditingSettingsPageWidget()
    {
        auto s = QmlJsEditingSettings::get();
        autoFormatOnSave = new QCheckBox(Tr::tr("Enable auto format on file save"));
        autoFormatOnSave->setChecked(s.autoFormatOnSave());
        autoFormatOnlyCurrentProject =
                new QCheckBox(Tr::tr("Restrict to files contained in the current project"));
        autoFormatOnlyCurrentProject->setChecked(s.autoFormatOnlyCurrentProject());
        autoFormatOnlyCurrentProject->setEnabled(autoFormatOnSave->isChecked());
        useCustomFormatCommand = new QCheckBox(
            Tr::tr("Use custom command instead of built-in formatter"));
        useCustomFormatCommand->setChecked(s.useCustomFormatCommand());
        auto formatCommandLabel = new QLabel(Tr::tr("Command:"));
        formatCommand = new QLineEdit();
        formatCommand->setText(s.formatCommand());
        formatCommand->setPlaceholderText(s.defaultFormatCommand());
        auto formatCommandOptionsLabel = new QLabel(Tr::tr("Arguments:"));
        formatCommandOptions = new QLineEdit();
        formatCommandOptions->setText(s.formatCommandOptions());
        pinContextPane = new QCheckBox(Tr::tr("Pin Qt Quick Toolbar"));
        pinContextPane->setChecked(s.pinContextPane());
        enableContextPane = new QCheckBox(Tr::tr("Always show Qt Quick Toolbar"));
        enableContextPane->setChecked(s.enableContextPane());
        foldAuxData = new QCheckBox(Tr::tr("Auto-fold auxiliary data"));
        foldAuxData->setChecked(s.foldAuxData());
        uiQmlOpenComboBox = new QComboBox;
        uiQmlOpenComboBox->addItem(Tr::tr("Always Ask"), "");
        uiQmlOpenComboBox->addItem(Tr::tr("Qt Design Studio"), Core::Constants::MODE_DESIGN);
        uiQmlOpenComboBox->addItem(Tr::tr("Qt Creator"), Core::Constants::MODE_EDIT);
        const int comboIndex = qMax(0, uiQmlOpenComboBox->findData(s.uiQmlOpenMode()));
        uiQmlOpenComboBox->setCurrentIndex(comboIndex);
        uiQmlOpenComboBox->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
        uiQmlOpenComboBox->setSizeAdjustPolicy(QComboBox::QComboBox::AdjustToContents);

        useQmlls = new QCheckBox(Tr::tr("Use qmlls (EXPERIMENTAL!)"));
        useQmlls->setChecked(s.qmllsSettings().useQmlls);
        useLatestQmlls = new QCheckBox(Tr::tr("Always use latest qmlls"));
        useLatestQmlls->setChecked(s.qmllsSettings().useLatestQmlls);
        useLatestQmlls->setEnabled(s.qmllsSettings().useQmlls);
        QObject::connect(useQmlls, &QCheckBox::stateChanged, this, [this](int checked) {
            useLatestQmlls->setEnabled(checked != Qt::Unchecked);
        });

        using namespace Layouting;
        // clang-format off
        QWidget *formattingGroup = nullptr;
        Column {
            Group {
                bindTo(&formattingGroup),
                title(Tr::tr("Automatic Formatting on File Save")),
                Column {
                    autoFormatOnSave,
                    autoFormatOnlyCurrentProject,
                    useCustomFormatCommand,
                    Form {
                        formatCommandLabel, formatCommand, br,
                        formatCommandOptionsLabel, formatCommandOptions
                    }
                },
            },
            Group {
                title(Tr::tr("Qt Quick Toolbars")),
                Column { pinContextPane, enableContextPane },
            },
            Group {
                title(Tr::tr("Features")),
                Column {
                    foldAuxData,
                    Form { Tr::tr("Open .ui.qml files with:"), uiQmlOpenComboBox },
                },
            },
            Group{
                title(Tr::tr("Language Server")),
                Column{useQmlls, useLatestQmlls},
            },
            st,
        }.attachTo(this);
        // clang-format on

        Utils::VariableChooser::addSupportForChildWidgets(formattingGroup,
                                                          Utils::globalMacroExpander());

        const auto updateFormatCommandState = [&, formatCommandLabel, formatCommandOptionsLabel] {
            const bool enabled = useCustomFormatCommand->isChecked();
            formatCommandLabel->setEnabled(enabled);
            formatCommand->setEnabled(enabled);
            formatCommandOptionsLabel->setEnabled(enabled);
            formatCommandOptions->setEnabled(enabled);
        };
        updateFormatCommandState();

        connect(autoFormatOnSave, &QCheckBox::toggled, this, [&, updateFormatCommandState]() {
            autoFormatOnlyCurrentProject->setEnabled(autoFormatOnSave->isChecked());
            updateFormatCommandState();
        });
        connect(useCustomFormatCommand, &QCheckBox::toggled, this, updateFormatCommandState);
    }

    void apply() final
    {
        QmlJsEditingSettings s;
        s.setEnableContextPane(enableContextPane->isChecked());
        s.setPinContextPane(pinContextPane->isChecked());
        s.setAutoFormatOnSave(autoFormatOnSave->isChecked());
        s.setAutoFormatOnlyCurrentProject(autoFormatOnlyCurrentProject->isChecked());
        s.setUseCustomFormatCommand(useCustomFormatCommand->isChecked());
        s.setFormatCommand(formatCommand->text());
        s.setFormatCommandOptions(formatCommandOptions->text());
        s.setFoldAuxData(foldAuxData->isChecked());
        s.setUiQmlOpenMode(uiQmlOpenComboBox->currentData().toString());
        s.qmllsSettings().useQmlls = useQmlls->isChecked();
        s.qmllsSettings().useLatestQmlls = useLatestQmlls->isChecked();
        s.set();
    }

private:
    QCheckBox *autoFormatOnSave;
    QCheckBox *autoFormatOnlyCurrentProject;
    QCheckBox *useCustomFormatCommand;
    QLineEdit *formatCommand;
    QLineEdit *formatCommandOptions;
    QCheckBox *pinContextPane;
    QCheckBox *enableContextPane;
    QCheckBox *foldAuxData;
    QCheckBox *useQmlls;
    QCheckBox *useLatestQmlls;
    QComboBox *uiQmlOpenComboBox;
};


QmlJsEditingSettings QmlJsEditingSettings::get()
{
    QmlJsEditingSettings settings;
    settings.fromSettings(Core::ICore::settings());
    return settings;
}

QmlJsEditingSettingsPage::QmlJsEditingSettingsPage()
{
    setId("C.QmlJsEditing");
    setDisplayName(Tr::tr("QML/JS Editing"));
    setCategory(Constants::SETTINGS_CATEGORY_QML);
    setWidgetCreator([] { return new QmlJsEditingSettingsPageWidget; });
}
