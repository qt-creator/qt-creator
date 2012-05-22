/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "cppsettingspage.h"
#include "designerconstants.h"

#include <QCoreApplication>
#include <QTextStream>
#include <coreplugin/icore.h>

namespace Designer {
namespace Internal {

// ---------- CppSettingsPageWidget

CppSettingsPageWidget::CppSettingsPageWidget(QWidget *parent) :
        QWidget(parent)
{
    m_ui.setupUi(this);
    connect(m_ui.includeQtModuleCheckBox, SIGNAL(toggled(bool)),
            m_ui.addQtVersionCheckBox, SLOT(setEnabled(bool)));
}

FormClassWizardGenerationParameters CppSettingsPageWidget::parameters() const
{
    FormClassWizardGenerationParameters rc;
    rc.embedding = static_cast<UiClassEmbedding>(uiEmbedding());
    rc.retranslationSupport =m_ui.retranslateCheckBox->isChecked();
    rc.includeQtModule = m_ui.includeQtModuleCheckBox->isChecked();
    rc.addQtVersionCheck = m_ui.addQtVersionCheckBox->isChecked();
    return rc;
}

void CppSettingsPageWidget::setParameters(const FormClassWizardGenerationParameters &p)
{
    m_ui.retranslateCheckBox->setChecked(p.retranslationSupport);
    m_ui.includeQtModuleCheckBox->setChecked(p.includeQtModule);
    m_ui.addQtVersionCheckBox->setChecked(p.addQtVersionCheck);
    setUiEmbedding(p.embedding);
}

int CppSettingsPageWidget::uiEmbedding() const
{
    if (m_ui.ptrAggregationRadioButton->isChecked())
        return PointerAggregatedUiClass;
    if (m_ui.aggregationButton->isChecked())
        return AggregatedUiClass;
    return InheritedUiClass;
}

void CppSettingsPageWidget::setUiEmbedding(int v)
{
    switch (v) {
    case PointerAggregatedUiClass:
        m_ui.ptrAggregationRadioButton->setChecked(true);
        break;
    case AggregatedUiClass:
        m_ui.aggregationButton->setChecked(true);
        break;
    case InheritedUiClass:
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
    m_parameters.fromSettings(Core::ICore::settings());
    setId(QLatin1String(Designer::Constants::SETTINGS_CPP_SETTINGS_ID));
    setDisplayName(QCoreApplication::translate("Designer", Designer::Constants::SETTINGS_CPP_SETTINGS_NAME));
    setCategory(QLatin1String(Designer::Constants::SETTINGS_CATEGORY));
    setDisplayCategory(QCoreApplication::translate("Designer", Designer::Constants::SETTINGS_TR_CATEGORY));
    setCategoryIcon(QLatin1String(Designer::Constants::SETTINGS_CATEGORY_ICON));
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
            m_parameters.toSettings(Core::ICore::settings());
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
