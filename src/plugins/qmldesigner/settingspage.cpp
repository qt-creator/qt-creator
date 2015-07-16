/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
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

SettingsPageWidget::SettingsPageWidget(QWidget *parent) :
    QWidget(parent)
{
    m_ui.setupUi(this);
    connect(m_ui.designerEnableDebuggerCheckBox, &QCheckBox::toggled,
            this, &SettingsPageWidget::debugViewEnabledToggled);
    m_ui.resetFallbackPuppetPathButton->hide();
    connect(m_ui.resetFallbackPuppetPathButton, &QPushButton::clicked, this, &SettingsPageWidget::resetFallbackPuppetPath);
    connect(m_ui.resetQmlPuppetBuildPathButton, &QPushButton::clicked, this, &SettingsPageWidget::resetQmlPuppetBuildPath);
    connect(m_ui.useDefaultPuppetRadioButton, &QRadioButton::toggled, m_ui.fallbackPuppetPathLineEdit, &QLineEdit::setEnabled);
    connect(m_ui.useQtRelatedPuppetRadioButton, &QRadioButton::toggled, m_ui.puppetBuildPathLineEdit, &QLineEdit::setEnabled);
    connect(m_ui.resetStyle, &QPushButton::clicked, this, &SettingsPageWidget::resetQmlStyle);
}

DesignerSettings SettingsPageWidget::settings() const
{
    DesignerSettings designerSettings;
    designerSettings.itemSpacing = m_ui.spinItemSpacing->value();
    designerSettings.containerPadding = m_ui.spinSnapMargin->value();
    designerSettings.canvasWidth = m_ui.spinCanvasWidth->value();
    designerSettings.canvasHeight = m_ui.spinCanvasHeight->value();
    designerSettings.warningsInDesigner = m_ui.designerWarningsCheckBox->isChecked();
    designerSettings.designerWarningsInEditor = m_ui.designerWarningsInEditorCheckBox->isChecked();
    designerSettings.showDebugView = m_ui.designerShowDebuggerCheckBox->isChecked();
    designerSettings.enableDebugView = m_ui.designerEnableDebuggerCheckBox->isChecked();
    designerSettings.useOnlyFallbackPuppet = m_ui.useDefaultPuppetRadioButton->isChecked();
    designerSettings.controlsStyle = m_ui.styleLineEdit->text();

    if (!m_ui.fallbackPuppetPathLineEdit->path().isEmpty() &&
        m_ui.fallbackPuppetPathLineEdit->path() != PuppetCreator::defaultPuppetFallbackDirectory()) {
        designerSettings.puppetFallbackDirectory = m_ui.fallbackPuppetPathLineEdit->path();
    }

    if (!m_ui.puppetBuildPathLineEdit->path().isEmpty() &&
        m_ui.puppetBuildPathLineEdit->path() != PuppetCreator::defaultPuppetToplevelBuildDirectory()) {
        designerSettings.puppetToplevelBuildDirectory = m_ui.puppetBuildPathLineEdit->path();
    }

    return designerSettings;
}

void SettingsPageWidget::setSettings(const DesignerSettings &designerSettings)
{
    m_ui.spinItemSpacing->setValue(designerSettings.itemSpacing);
    m_ui.spinSnapMargin->setValue(designerSettings.containerPadding);
    m_ui.spinCanvasWidth->setValue(designerSettings.canvasWidth);
    m_ui.spinCanvasHeight->setValue(designerSettings.canvasHeight);
    m_ui.designerWarningsCheckBox->setChecked(designerSettings.warningsInDesigner);
    m_ui.designerWarningsInEditorCheckBox->setChecked(designerSettings.designerWarningsInEditor);
    m_ui.designerShowDebuggerCheckBox->setChecked(designerSettings.showDebugView);
    m_ui.designerEnableDebuggerCheckBox->setChecked(designerSettings.enableDebugView);
    m_ui.useDefaultPuppetRadioButton->setChecked(designerSettings.useOnlyFallbackPuppet);
    m_ui.useQtRelatedPuppetRadioButton->setChecked(!designerSettings.useOnlyFallbackPuppet);
    m_ui.styleLineEdit->setText(designerSettings.controlsStyle);

    if (designerSettings.puppetFallbackDirectory.isEmpty())
        resetFallbackPuppetPath();
    else
        m_ui.fallbackPuppetPathLineEdit->setPath(designerSettings.puppetFallbackDirectory);

    if (designerSettings.puppetToplevelBuildDirectory.isEmpty())
        resetQmlPuppetBuildPath();
    else
        m_ui.puppetBuildPathLineEdit->setPath(designerSettings.puppetToplevelBuildDirectory);
}

void SettingsPageWidget::resetFallbackPuppetPath()
{
    m_ui.fallbackPuppetPathLineEdit->setPath(PuppetCreator::defaultPuppetFallbackDirectory());
}

void SettingsPageWidget::resetQmlPuppetBuildPath()
{
    m_ui.puppetBuildPathLineEdit->setPath(PuppetCreator::defaultPuppetToplevelBuildDirectory());
}

void SettingsPageWidget::resetQmlStyle()
{
    m_ui.styleLineEdit->setText(QString());
}

void SettingsPageWidget::debugViewEnabledToggled(bool b)
{
    if (b && ! m_ui.designerShowDebuggerCheckBox->isChecked())
        m_ui.designerShowDebuggerCheckBox->setChecked(true);
}

SettingsPage::SettingsPage() :
    m_widget(0)
{
    setId("B.QmlDesigner");
    setDisplayName(tr("Qt Quick Designer"));
    setCategory(QmlJSEditor::Constants::SETTINGS_CATEGORY_QML);
    setDisplayCategory(QCoreApplication::translate("QmlJSEditor",
        QmlJSEditor::Constants::SETTINGS_TR_CATEGORY_QML));
    setCategoryIcon(QLatin1String(QmlJSTools::Constants::SETTINGS_CATEGORY_QML_ICON));
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

    DesignerSettings currentDesignerSettings(QmlDesignerPlugin::instance()->settings());
    DesignerSettings newDesignerSettings(m_widget->settings());

    if (currentDesignerSettings.puppetFallbackDirectory != newDesignerSettings.puppetFallbackDirectory ||
        currentDesignerSettings.puppetToplevelBuildDirectory != newDesignerSettings.puppetToplevelBuildDirectory) {
        QMessageBox::information(Core::ICore::mainWindow(), tr("Restart Required"),
            tr("The QML emulation layer path changes will take effect after a restart of the QML Emulation layer or Qt Creator."));
    }

    QmlDesignerPlugin::instance()->setSettings(newDesignerSettings);
}

void SettingsPage::finish()
{
    delete m_widget;
}
