/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "cppsettingspage.h"
#include "designerconstants.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QTextStream>
#include <coreplugin/icore.h>

namespace Designer {
namespace Internal {

// ---------- CppSettingsPageWidget

CppSettingsPageWidget::CppSettingsPageWidget(QWidget *parent) :
        QWidget(parent)
{
    m_ui.setupUi(this);
}

FormClassWizardGenerationParameters CppSettingsPageWidget::parameters() const
{
    FormClassWizardGenerationParameters rc;
    rc.setEmbedding(static_cast<FormClassWizardGenerationParameters::UiClassEmbedding>(uiEmbedding()));
    rc.setRetranslationSupport(m_ui.retranslateCheckBox->isChecked());
    rc.setIncludeQtModule(m_ui.includeQtModuleCheckBox->isChecked());
    return rc;
}

void CppSettingsPageWidget::setParameters(const FormClassWizardGenerationParameters &p)
{
    m_ui.retranslateCheckBox->setChecked(p.retranslationSupport());
    m_ui.includeQtModuleCheckBox->setChecked(p.includeQtModule());
    setUiEmbedding(p.embedding());
}

int CppSettingsPageWidget::uiEmbedding() const
{
    if (m_ui.ptrAggregationRadioButton->isChecked())
        return FormClassWizardGenerationParameters::PointerAggregatedUiClass;
    if (m_ui.aggregationButton->isChecked())
        return FormClassWizardGenerationParameters::AggregatedUiClass;
    return FormClassWizardGenerationParameters::InheritedUiClass;
}

void CppSettingsPageWidget::setUiEmbedding(int v)
{
    switch (v) {
    case FormClassWizardGenerationParameters::PointerAggregatedUiClass:
        m_ui.ptrAggregationRadioButton->setChecked(true);
        break;
    case FormClassWizardGenerationParameters::AggregatedUiClass:
        m_ui.aggregationButton->setChecked(true);
        break;
    case FormClassWizardGenerationParameters::InheritedUiClass:
        m_ui.multipleInheritanceButton->setChecked(true);
        break;
    }
}

QString CppSettingsPageWidget::searchKeywords() const
{
    QString rc;
    QTextStream(&rc) << m_ui.ptrAggregationRadioButton->text()
            << ' ' << m_ui.aggregationButton->text()
            << ' ' << m_ui.multipleInheritanceButton->text()
            << ' ' << m_ui.retranslateCheckBox->text()
            << ' ' << m_ui.includeQtModuleCheckBox->text();
    rc.remove(QLatin1Char('&'));
    return rc;
}

// ---------- CppSettingsPage
CppSettingsPage::CppSettingsPage(QObject *parent) : Core::IOptionsPage(parent)
{
    m_parameters.fromSettings(Core::ICore::instance()->settings());
}

QString CppSettingsPage::id() const
{
    return QLatin1String(Designer::Constants::SETTINGS_CPP_SETTINGS_ID);
}

QString CppSettingsPage::displayName() const
{
    return QCoreApplication::translate("Designer", Designer::Constants::SETTINGS_CPP_SETTINGS_NAME);
}

QString CppSettingsPage::category() const
{
    return QLatin1String(Designer::Constants::SETTINGS_CATEGORY);
}

QString CppSettingsPage::displayCategory() const
{
    return QCoreApplication::translate("Designer", Designer::Constants::SETTINGS_TR_CATEGORY);
}

QWidget *CppSettingsPage::createPage(QWidget *parent)
{
    m_widget = new CppSettingsPageWidget(parent);
    m_widget->setParameters(m_parameters);
    if (m_searchKeywords.isEmpty())
        m_searchKeywords = m_widget->searchKeywords();
    return m_widget;
}

void CppSettingsPage::apply()
{
    if (m_widget) {
        const FormClassWizardGenerationParameters newParameters = m_widget->parameters();
        if (newParameters != m_parameters) {
            m_parameters = newParameters;
            m_parameters.toSettings(Core::ICore::instance()->settings());
        }
    }
}

void CppSettingsPage::finish()
{
}

bool CppSettingsPage::matches(const QString &s) const
{
    return m_searchKeywords.contains(s, Qt::CaseInsensitive);
}

} // namespace Internal
} // namespace Designer
