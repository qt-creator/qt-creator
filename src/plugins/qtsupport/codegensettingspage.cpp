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

#include "codegensettingspage.h"

#include "qtsupportconstants.h"

#include <cpptools/cpptoolsconstants.h>

#include <QCoreApplication>
#include <QTextStream>
#include <coreplugin/icore.h>

namespace QtSupport {
namespace Internal {

// ---------- CodeGenSettingsPageWidget

CodeGenSettingsPageWidget::CodeGenSettingsPageWidget(QWidget *parent) :
        QWidget(parent)
{
    m_ui.setupUi(this);
    connect(m_ui.includeQtModuleCheckBox, &QAbstractButton::toggled,
            m_ui.addQtVersionCheckBox, &QWidget::setEnabled);
}

CodeGenSettings CodeGenSettingsPageWidget::parameters() const
{
    CodeGenSettings rc;
    rc.embedding = static_cast<CodeGenSettings::UiClassEmbedding>(uiEmbedding());
    rc.retranslationSupport =m_ui.retranslateCheckBox->isChecked();
    rc.includeQtModule = m_ui.includeQtModuleCheckBox->isChecked();
    rc.addQtVersionCheck = m_ui.addQtVersionCheckBox->isChecked();
    return rc;
}

void CodeGenSettingsPageWidget::setParameters(const CodeGenSettings &p)
{
    m_ui.retranslateCheckBox->setChecked(p.retranslationSupport);
    m_ui.includeQtModuleCheckBox->setChecked(p.includeQtModule);
    m_ui.addQtVersionCheckBox->setChecked(p.addQtVersionCheck);
    setUiEmbedding(p.embedding);
}

int CodeGenSettingsPageWidget::uiEmbedding() const
{
    if (m_ui.ptrAggregationRadioButton->isChecked())
        return CodeGenSettings::PointerAggregatedUiClass;
    if (m_ui.aggregationButton->isChecked())
        return CodeGenSettings::AggregatedUiClass;
    return CodeGenSettings::InheritedUiClass;
}

void CodeGenSettingsPageWidget::setUiEmbedding(int v)
{
    switch (v) {
    case CodeGenSettings::PointerAggregatedUiClass:
        m_ui.ptrAggregationRadioButton->setChecked(true);
        break;
    case CodeGenSettings::AggregatedUiClass:
        m_ui.aggregationButton->setChecked(true);
        break;
    case CodeGenSettings::InheritedUiClass:
        m_ui.multipleInheritanceButton->setChecked(true);
        break;
    }
}

// ---------- CodeGenSettingsPage
CodeGenSettingsPage::CodeGenSettingsPage(QObject *parent) :
    Core::IOptionsPage(parent),
    m_widget(0)
{
    m_parameters.fromSettings(Core::ICore::settings());
    setId(Constants::CODEGEN_SETTINGS_PAGE_ID);
    setDisplayName(QCoreApplication::translate("QtSupport", Constants::CODEGEN_SETTINGS_PAGE_NAME));
    setCategory(CppTools::Constants::CPP_SETTINGS_CATEGORY);
    setDisplayCategory(QCoreApplication::translate("CppTools", CppTools::Constants::CPP_SETTINGS_TR_CATEGORY));
    setCategoryIcon(Utils::Icon(CppTools::Constants::SETTINGS_CATEGORY_CPP_ICON));
}

QWidget *CodeGenSettingsPage::widget()
{
    if (!m_widget) {
        m_widget = new CodeGenSettingsPageWidget;
        m_widget->setParameters(m_parameters);
    }
    return m_widget;
}

void CodeGenSettingsPage::apply()
{
    if (m_widget) {
        const CodeGenSettings newParameters = m_widget->parameters();
        if (newParameters != m_parameters) {
            m_parameters = newParameters;
            m_parameters.toSettings(Core::ICore::settings());
        }
    }
}

void CodeGenSettingsPage::finish()
{
    delete m_widget;
}

} // namespace Internal
} // namespace QtSupport
