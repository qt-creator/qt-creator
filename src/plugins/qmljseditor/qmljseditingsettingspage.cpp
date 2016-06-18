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

#include "qmljseditingsettingspage.h"
#include "qmljseditorconstants.h"

#include <qmljstools/qmljstoolsconstants.h>
#include <coreplugin/icore.h>

#include <QSettings>
#include <QTextStream>
#include <QCheckBox>


using namespace QmlJSEditor;
using namespace QmlJSEditor::Internal;

QmlJsEditingSettings::QmlJsEditingSettings()
    : m_enableContextPane(false),
    m_pinContextPane(false),
    m_autoFormatOnSave(false),
    m_autoFormatOnlyCurrentProject(false)
{}

void QmlJsEditingSettings::set()
{
    if (get() != *this)
        toSettings(Core::ICore::settings());
}

void QmlJsEditingSettings::fromSettings(QSettings *settings)
{
    settings->beginGroup(QLatin1String(QmlJSEditor::Constants::SETTINGS_CATEGORY_QML));
    m_enableContextPane = settings->value(
                QLatin1String(QmlJSEditor::Constants::QML_CONTEXTPANE_KEY),
                QVariant(false)).toBool();
    m_pinContextPane = settings->value(
                QLatin1String(QmlJSEditor::Constants::QML_CONTEXTPANEPIN_KEY),
                QVariant(false)).toBool();
    m_autoFormatOnSave = settings->value(
                QLatin1String(QmlJSEditor::Constants::AUTO_FORMAT_ON_SAVE),
                QVariant(false)).toBool();
    m_autoFormatOnlyCurrentProject = settings->value(
                QLatin1String(QmlJSEditor::Constants::AUTO_FORMAT_ONLY_CURRENT_PROJECT),
                QVariant(false)).toBool();
    settings->endGroup();
}

void QmlJsEditingSettings::toSettings(QSettings *settings) const
{
    settings->beginGroup(QLatin1String(QmlJSEditor::Constants::SETTINGS_CATEGORY_QML));
    settings->setValue(QLatin1String(QmlJSEditor::Constants::QML_CONTEXTPANE_KEY),
                       m_enableContextPane);
    settings->setValue(QLatin1String(QmlJSEditor::Constants::QML_CONTEXTPANEPIN_KEY),
                       m_pinContextPane);
    settings->setValue(QLatin1String(QmlJSEditor::Constants::AUTO_FORMAT_ON_SAVE),
                       m_autoFormatOnSave);
    settings->setValue(QLatin1String(QmlJSEditor::Constants::AUTO_FORMAT_ONLY_CURRENT_PROJECT),
                       m_autoFormatOnlyCurrentProject);
    settings->endGroup();
}

bool QmlJsEditingSettings::equals(const QmlJsEditingSettings &other) const
{
    return  m_enableContextPane == other.m_enableContextPane
            && m_pinContextPane == other.m_pinContextPane
            && m_autoFormatOnSave == other.m_autoFormatOnSave
            && m_autoFormatOnlyCurrentProject == other.m_autoFormatOnlyCurrentProject;
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

QmlJsEditingSettignsPageWidget::QmlJsEditingSettignsPageWidget(QWidget *parent) :
    QWidget(parent)
{
    m_ui.setupUi(this);
}

QmlJsEditingSettings QmlJsEditingSettignsPageWidget::settings() const
{
    QmlJsEditingSettings s;
    s.setEnableContextPane(m_ui.textEditHelperCheckBox->isChecked());
    s.setPinContextPane(m_ui.textEditHelperCheckBoxPin->isChecked());
    s.setAutoFormatOnSave(m_ui.autoFormatOnSave->isChecked());
    s.setAutoFormatOnlyCurrentProject(m_ui.autoFormatOnlyCurrentProject->isChecked());
    return s;
}

void QmlJsEditingSettignsPageWidget::setSettings(const QmlJsEditingSettings &s)
{
    m_ui.textEditHelperCheckBox->setChecked(s.enableContextPane());
    m_ui.textEditHelperCheckBoxPin->setChecked(s.pinContextPane());
    m_ui.autoFormatOnSave->setChecked(s.autoFormatOnSave());
    m_ui.autoFormatOnlyCurrentProject->setChecked(s.autoFormatOnlyCurrentProject());
}

QmlJsEditingSettings QmlJsEditingSettings::get()
{
    QmlJsEditingSettings settings;
    settings.fromSettings(Core::ICore::settings());
    return settings;
}

QmlJsEditingSettingsPage::QmlJsEditingSettingsPage() :
    m_widget(0)
{
    setId("C.QmlJsEditing");
    setDisplayName(tr("QML/JS Editing"));
    setCategory(Constants::SETTINGS_CATEGORY_QML);
    setDisplayCategory(QCoreApplication::translate("QmlJSEditor",
        QmlJSEditor::Constants::SETTINGS_TR_CATEGORY_QML));
    setCategoryIcon(Utils::Icon(QmlJSTools::Constants::SETTINGS_CATEGORY_QML_ICON));
}

QWidget *QmlJsEditingSettingsPage::widget()
{
    if (!m_widget) {
        m_widget = new QmlJsEditingSettignsPageWidget;
        m_widget->setSettings(QmlJsEditingSettings::get());
    }
    return m_widget;
}

void QmlJsEditingSettingsPage::apply()
{
    if (!m_widget) // page was never shown
        return;
    m_widget->settings().set();
}

void QmlJsEditingSettingsPage::finish()
{
    delete m_widget;
}
