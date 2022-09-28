// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "qmljseditingsettingspage.h"
#include "qmljseditorconstants.h"
#include "qmljseditortr.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/icore.h>
#include <qmljstools/qmljstoolsconstants.h>
#include <utils/layoutbuilder.h>

#include <QCheckBox>
#include <QComboBox>
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
    settings->endGroup();
    QmllsSettingsManager::instance()->checkForChanges();
}

bool QmlJsEditingSettings::equals(const QmlJsEditingSettings &other) const
{
    return m_enableContextPane == other.m_enableContextPane
           && m_pinContextPane == other.m_pinContextPane
           && m_autoFormatOnSave == other.m_autoFormatOnSave
           && m_autoFormatOnlyCurrentProject == other.m_autoFormatOnlyCurrentProject
           && m_foldAuxData == other.m_foldAuxData
           && m_qmllsSettings == other.m_qmllsSettings
           && m_uiQmlOpenMode == other.m_uiQmlOpenMode;
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

QmllsSettings &QmlJsEditingSettings::qmllsSettigs()
{
    return m_qmllsSettings;
}

const QmllsSettings &QmlJsEditingSettings::qmllsSettigs() const
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

        useQmlls = new QCheckBox(tr("Use qmlls (EXPERIMENTAL!)"));
        useQmlls->setChecked(s.qmllsSettigs().useQmlls);
        useLatestQmlls = new QCheckBox(tr("Always use latest qmlls"));
        useLatestQmlls->setChecked(s.qmllsSettigs().useLatestQmlls);
        useLatestQmlls->setEnabled(s.qmllsSettigs().useQmlls);
        QObject::connect(useQmlls, &QCheckBox::stateChanged, this, [this](int checked) {
            useLatestQmlls->setEnabled(checked != Qt::Unchecked);
        });
        using namespace Utils::Layouting;
        Column {
            Group {
                title(Tr::tr("Automatic Formatting on File Save")),
                Column { autoFormatOnSave, autoFormatOnlyCurrentProject },
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
                title(tr("Language Server")),
                Column{useQmlls, useLatestQmlls},
            },
            st,
        }
            .attachTo(this);

        connect(autoFormatOnSave, &QCheckBox::toggled,
                autoFormatOnlyCurrentProject, &QWidget::setEnabled);
    }

    void apply() final
    {
        QmlJsEditingSettings s;
        s.setEnableContextPane(enableContextPane->isChecked());
        s.setPinContextPane(pinContextPane->isChecked());
        s.setAutoFormatOnSave(autoFormatOnSave->isChecked());
        s.setAutoFormatOnlyCurrentProject(autoFormatOnlyCurrentProject->isChecked());
        s.setFoldAuxData(foldAuxData->isChecked());
        s.setUiQmlOpenMode(uiQmlOpenComboBox->currentData().toString());
        s.qmllsSettigs().useQmlls = useQmlls->isChecked();
        s.qmllsSettigs().useLatestQmlls = useLatestQmlls->isChecked();
        s.set();
    }

private:
    QCheckBox *autoFormatOnSave;
    QCheckBox *autoFormatOnlyCurrentProject;
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
