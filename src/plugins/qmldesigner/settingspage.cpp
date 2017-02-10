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

#include "settingspage.h"
#include "qmldesignerconstants.h"
#include "qmldesignerplugin.h"
#include "designersettings.h"
#include "puppetcreator.h"

#include <coreplugin/icore.h>

#include <qmljseditor/qmljseditorconstants.h>
#include <qmljstools/qmljstoolsconstants.h>

#include <QLineEdit>
#include <QTextStream>
#include <QMessageBox>

using namespace QmlDesigner;
using namespace QmlDesigner::Internal;

namespace {
    QStringList puppetModes()
    {
        static QStringList puppetModeList{QLatin1String(""), QLatin1String("all"),
            QLatin1String("editormode"), QLatin1String("rendermode"), QLatin1String("previewmode")};
        return puppetModeList;
    }
}

SettingsPageWidget::SettingsPageWidget(QWidget *parent) :
    QWidget(parent)
{
    m_ui.setupUi(this);

    connect(m_ui.designerEnableDebuggerCheckBox, &QCheckBox::toggled, [=](bool checked) {
        if (checked && ! m_ui.designerShowDebuggerCheckBox->isChecked())
            m_ui.designerShowDebuggerCheckBox->setChecked(true);
        }
    );
    m_ui.resetFallbackPuppetPathButton->hide();
    connect(m_ui.resetFallbackPuppetPathButton, &QPushButton::clicked, [=]() {
        m_ui.fallbackPuppetPathLineEdit->setPath(
            PuppetCreator::defaultPuppetFallbackDirectory());
        }
    );
    connect(m_ui.resetQmlPuppetBuildPathButton, &QPushButton::clicked, [=]() {
        m_ui.puppetBuildPathLineEdit->setPath(
            PuppetCreator::defaultPuppetToplevelBuildDirectory());
        }
    );
    connect(m_ui.useDefaultPuppetRadioButton, &QRadioButton::toggled,
        m_ui.fallbackPuppetPathLineEdit, &QLineEdit::setEnabled);
    connect(m_ui.useQtRelatedPuppetRadioButton, &QRadioButton::toggled,
        m_ui.puppetBuildPathLineEdit, &QLineEdit::setEnabled);
    connect(m_ui.resetStyle, &QPushButton::clicked,
        m_ui.styleLineEdit, &QLineEdit::clear);
    connect(m_ui.controls2StyleComboBox, &QComboBox::currentTextChanged, [=]() {
            m_ui.styleLineEdit->setText(m_ui.controls2StyleComboBox->currentText());
        }
    );

    m_ui.forwardPuppetOutputComboBox->addItems(puppetModes());
    m_ui.debugPuppetComboBox->addItems(puppetModes());
}

DesignerSettings SettingsPageWidget::settings() const
{
    DesignerSettings settings = QmlDesignerPlugin::instance()->settings();
    settings.insert(DesignerSettingsKey::ITEMSPACING, m_ui.spinItemSpacing->value());
    settings.insert(DesignerSettingsKey::CONTAINERPADDING, m_ui.spinSnapMargin->value());
    settings.insert(DesignerSettingsKey::CANVASWIDTH, m_ui.spinCanvasWidth->value());
    settings.insert(DesignerSettingsKey::CANVASHEIGHT, m_ui.spinCanvasHeight->value());
    settings.insert(DesignerSettingsKey::WARNING_FOR_FEATURES_IN_DESIGNER,
                    m_ui.designerWarningsCheckBox->isChecked());
    settings.insert(DesignerSettingsKey::WARNING_FOR_QML_FILES_INSTEAD_OF_UIQML_FILES,
                    m_ui.designerWarningsUiQmlfiles->isChecked());

    settings.insert(DesignerSettingsKey::WARNING_FOR_DESIGNER_FEATURES_IN_EDITOR,
        m_ui.designerWarningsInEditorCheckBox->isChecked());
    settings.insert(DesignerSettingsKey::SHOW_DEBUGVIEW,
        m_ui.designerShowDebuggerCheckBox->isChecked());
    settings.insert(DesignerSettingsKey::ENABLE_DEBUGVIEW,
        m_ui.designerEnableDebuggerCheckBox->isChecked());
    settings.insert(DesignerSettingsKey::USE_ONLY_FALLBACK_PUPPET,
        m_ui.useDefaultPuppetRadioButton->isChecked());

    int typeOfQsTrFunction;

    if (m_ui.useQsTrFunctionRadioButton->isChecked())
        typeOfQsTrFunction = 0;
    else if (m_ui.useQsTrIdFunctionRadioButton->isChecked())
        typeOfQsTrFunction = 1;
    else if (m_ui.useQsTranslateFunctionRadioButton->isChecked())
        typeOfQsTrFunction = 2;
    else
        typeOfQsTrFunction = 0;

    settings.insert(DesignerSettingsKey::TYPE_OF_QSTR_FUNCTION, typeOfQsTrFunction);
    settings.insert(DesignerSettingsKey::CONTROLS_STYLE, m_ui.styleLineEdit->text());
    settings.insert(DesignerSettingsKey::FORWARD_PUPPET_OUTPUT,
        m_ui.forwardPuppetOutputComboBox->currentText());
    settings.insert(DesignerSettingsKey::DEBUG_PUPPET,
        m_ui.debugPuppetComboBox->currentText());

    if (!m_ui.fallbackPuppetPathLineEdit->path().isEmpty() &&
        m_ui.fallbackPuppetPathLineEdit->path() != PuppetCreator::defaultPuppetFallbackDirectory()) {
        settings.insert(DesignerSettingsKey::PUPPET_FALLBACK_DIRECTORY,
            m_ui.fallbackPuppetPathLineEdit->path());
    }

    if (!m_ui.puppetBuildPathLineEdit->path().isEmpty() &&
        m_ui.puppetBuildPathLineEdit->path() != PuppetCreator::defaultPuppetToplevelBuildDirectory()) {
        settings.insert(DesignerSettingsKey::PUPPET_TOPLEVEL_BUILD_DIRECTORY,
            m_ui.puppetBuildPathLineEdit->path());
    }
    settings.insert(DesignerSettingsKey::ALWAYS_SAFE_IN_CRUMBLEBAR,
        m_ui.alwaysSaveSubcomponentsCheckBox->isChecked());
    settings.insert(DesignerSettingsKey::SHOW_PROPERTYEDITOR_WARNINGS,
        m_ui.showPropertyEditorWarningsCheckBox->isChecked());
    settings.insert(DesignerSettingsKey::ENABLE_MODEL_EXCEPTION_OUTPUT,
        m_ui.showWarnExceptionsCheckBox->isChecked());

    return settings;
}

void SettingsPageWidget::setSettings(const DesignerSettings &settings)
{
    m_ui.spinItemSpacing->setValue(settings.value(
        DesignerSettingsKey::ITEMSPACING).toInt());
    m_ui.spinSnapMargin->setValue(settings.value(
        DesignerSettingsKey::CONTAINERPADDING).toInt());
    m_ui.spinCanvasWidth->setValue(settings.value(
        DesignerSettingsKey::CANVASWIDTH).toInt());
    m_ui.spinCanvasHeight->setValue(settings.value(
        DesignerSettingsKey::CANVASHEIGHT).toInt());
    m_ui.designerWarningsCheckBox->setChecked(settings.value(
        DesignerSettingsKey::WARNING_FOR_FEATURES_IN_DESIGNER).toBool());
    m_ui.designerWarningsUiQmlfiles->setChecked(settings.value(
        DesignerSettingsKey::WARNING_FOR_QML_FILES_INSTEAD_OF_UIQML_FILES).toBool());
    m_ui.designerWarningsInEditorCheckBox->setChecked(settings.value(
        DesignerSettingsKey::WARNING_FOR_DESIGNER_FEATURES_IN_EDITOR).toBool());
    m_ui.designerShowDebuggerCheckBox->setChecked(settings.value(
        DesignerSettingsKey::SHOW_DEBUGVIEW).toBool());
    m_ui.designerEnableDebuggerCheckBox->setChecked(settings.value(
        DesignerSettingsKey::ENABLE_DEBUGVIEW).toBool());
    m_ui.useDefaultPuppetRadioButton->setChecked(settings.value(
        DesignerSettingsKey::USE_ONLY_FALLBACK_PUPPET).toBool());
    m_ui.useQtRelatedPuppetRadioButton->setChecked(!settings.value(
        DesignerSettingsKey::USE_ONLY_FALLBACK_PUPPET).toBool());
    m_ui.useQsTrFunctionRadioButton->setChecked(settings.value(
        DesignerSettingsKey::TYPE_OF_QSTR_FUNCTION).toInt() == 0);
    m_ui.useQsTrIdFunctionRadioButton->setChecked(settings.value(
        DesignerSettingsKey::TYPE_OF_QSTR_FUNCTION).toInt() == 1);
    m_ui.useQsTranslateFunctionRadioButton->setChecked(settings.value(
        DesignerSettingsKey::TYPE_OF_QSTR_FUNCTION).toInt() == 2);
    m_ui.styleLineEdit->setText(settings.value(
        DesignerSettingsKey::CONTROLS_STYLE).toString());

    QString puppetFallbackDirectory = settings.value(
        DesignerSettingsKey::PUPPET_FALLBACK_DIRECTORY,
        PuppetCreator::defaultPuppetFallbackDirectory()).toString();
    m_ui.fallbackPuppetPathLineEdit->setPath(puppetFallbackDirectory);

    QString puppetToplevelBuildDirectory = settings.value(
        DesignerSettingsKey::PUPPET_TOPLEVEL_BUILD_DIRECTORY,
        PuppetCreator::defaultPuppetToplevelBuildDirectory()).toString();
    m_ui.puppetBuildPathLineEdit->setPath(puppetToplevelBuildDirectory);

    m_ui.forwardPuppetOutputComboBox->setCurrentText(settings.value(
        DesignerSettingsKey::FORWARD_PUPPET_OUTPUT).toString());
    m_ui.debugPuppetComboBox->setCurrentText(settings.value(
        DesignerSettingsKey::DEBUG_PUPPET).toString());

    m_ui.alwaysSaveSubcomponentsCheckBox->setChecked(settings.value(
        DesignerSettingsKey::ALWAYS_SAFE_IN_CRUMBLEBAR).toBool());
    m_ui.showPropertyEditorWarningsCheckBox->setChecked(settings.value(
        DesignerSettingsKey::SHOW_PROPERTYEDITOR_WARNINGS).toBool());
    m_ui.showWarnExceptionsCheckBox->setChecked(settings.value(
        DesignerSettingsKey::ENABLE_MODEL_EXCEPTION_OUTPUT).toBool());

    m_ui.controls2StyleComboBox->setCurrentText(m_ui.styleLineEdit->text());
}

SettingsPage::SettingsPage() :
    m_widget(0)
{
    setId("B.QmlDesigner");
    setDisplayName(tr("Qt Quick Designer"));
    setCategory(QmlJSEditor::Constants::SETTINGS_CATEGORY_QML);
    setDisplayCategory(QCoreApplication::translate("QmlJSEditor",
        QmlJSEditor::Constants::SETTINGS_TR_CATEGORY_QML));
    setCategoryIcon(Utils::Icon(QmlJSTools::Constants::SETTINGS_CATEGORY_QML_ICON));
}

QWidget *SettingsPage::widget()
{
    if (!m_widget) {
        m_widget = new SettingsPageWidget;
        m_widget->setSettings(QmlDesignerPlugin::instance()->settings());
    }
    return m_widget;
}

void SettingsPage::apply()
{
    if (!m_widget) // page was never shown
        return;

    DesignerSettings currentSettings(QmlDesignerPlugin::instance()->settings());
    DesignerSettings newSettings(m_widget->settings());

    QList<QByteArray> restartNecessaryKeys;
    restartNecessaryKeys << DesignerSettingsKey::PUPPET_FALLBACK_DIRECTORY
                         << DesignerSettingsKey::PUPPET_TOPLEVEL_BUILD_DIRECTORY
                         << DesignerSettingsKey::ENABLE_MODEL_EXCEPTION_OUTPUT
                         << DesignerSettingsKey::PUPPET_KILL_TIMEOUT
                         << DesignerSettingsKey::FORWARD_PUPPET_OUTPUT
                         << DesignerSettingsKey::DEBUG_PUPPET
                         << DesignerSettingsKey::ENABLE_MODEL_EXCEPTION_OUTPUT;

    foreach (const QByteArray &key, restartNecessaryKeys) {
        if (currentSettings.value(key) != newSettings.value(key)) {
            QMessageBox::information(Core::ICore::mainWindow(), tr("Restart Required"),
                tr("The made changes will take effect after a "
                   "restart of the QML Emulation layer or Qt Creator."));
            break;
        }
    }

    QmlDesignerPlugin::instance()->setSettings(newSettings);
}

void SettingsPage::finish()
{
    delete m_widget;
}
