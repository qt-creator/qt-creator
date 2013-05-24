/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "settingspage.h"
#include "designersettings.h"
#include "qmldesignerconstants.h"
#include "qmldesignerplugin.h"

#include <qmljseditor/qmljseditorconstants.h>

#include <QTextStream>

using namespace QmlDesigner;
using namespace QmlDesigner::Internal;

SettingsPageWidget::SettingsPageWidget(QWidget *parent) :
    QWidget(parent)
{
    m_ui.setupUi(this);
    connect(m_ui.designerEnableDebuggerCheckBox, SIGNAL(toggled(bool)), this, SLOT(debugViewEnabledToggled(bool)));
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
}

QString SettingsPageWidget::searchKeywords() const
{
    QString rc;
    QTextStream(&rc)
            << ' ' << m_ui.snapMarginLabel->text()
            << ' ' << m_ui.itemSpacingLabel->text()
            << ' ' << m_ui.canvasWidthLabel->text()
            << ' ' << m_ui.canvasHeightLabel->text();
    rc.remove(QLatin1Char('&'));
    return rc;
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
    setCategoryIcon(QLatin1String(Constants::SETTINGS_CATEGORY_QML_ICON));
}

QWidget *SettingsPage::createPage(QWidget *parent)
{
    m_widget = new SettingsPageWidget(parent);
    m_widget->setSettings(QmlDesignerPlugin::instance()->settings());
    if (m_searchKeywords.isEmpty())
        m_searchKeywords = m_widget->searchKeywords();
    return m_widget;
}

void SettingsPage::apply()
{
    if (!m_widget) // page was never shown
        return;
    QmlDesignerPlugin::instance()->setSettings(m_widget->settings());
}

bool SettingsPage::matches(const QString &s) const
{
    return m_searchKeywords.contains(s, Qt::CaseInsensitive);
}
