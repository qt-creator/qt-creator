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

#include "codegensettings.h"
#include "qtsupportconstants.h"
#include "ui_codegensettingspagewidget.h"

#include <coreplugin/icore.h>
#include <cpptools/cpptoolsconstants.h>

#include <QCoreApplication>

namespace QtSupport {
namespace Internal {

// ---------- CodeGenSettingsPageWidget

class CodeGenSettingsPageWidget : public Core::IOptionsPageWidget
{
    Q_DECLARE_TR_FUNCTIONS(QtSupport::Internal::CodeGenSettingsPage)

public:
    CodeGenSettingsPageWidget();

private:
    void apply() final;

    int uiEmbedding() const;
    void setUiEmbedding(int);

    Ui::CodeGenSettingsPageWidget m_ui;
};

CodeGenSettingsPageWidget::CodeGenSettingsPageWidget()
{
    m_ui.setupUi(this);

    CodeGenSettings parameters;
    parameters.fromSettings(Core::ICore::settings());

    m_ui.retranslateCheckBox->setChecked(parameters.retranslationSupport);
    m_ui.includeQtModuleCheckBox->setChecked(parameters.includeQtModule);
    m_ui.addQtVersionCheckBox->setChecked(parameters.addQtVersionCheck);
    setUiEmbedding(parameters.embedding);

    connect(m_ui.includeQtModuleCheckBox, &QAbstractButton::toggled,
            m_ui.addQtVersionCheckBox, &QWidget::setEnabled);
}

void CodeGenSettingsPageWidget::apply()
{
    CodeGenSettings rc;
    rc.embedding = static_cast<CodeGenSettings::UiClassEmbedding>(uiEmbedding());
    rc.retranslationSupport = m_ui.retranslateCheckBox->isChecked();
    rc.includeQtModule = m_ui.includeQtModuleCheckBox->isChecked();
    rc.addQtVersionCheck = m_ui.addQtVersionCheckBox->isChecked();
    rc.toSettings(Core::ICore::settings());
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

CodeGenSettingsPage::CodeGenSettingsPage()
{
    setId(Constants::CODEGEN_SETTINGS_PAGE_ID);
    setDisplayName(QCoreApplication::translate("QtSupport", "Qt Class Generation"));
    setCategory(CppTools::Constants::CPP_SETTINGS_CATEGORY);
    setDisplayCategory(
        QCoreApplication::translate("CppTools", CppTools::Constants::CPP_SETTINGS_NAME));
    setCategoryIconPath(":/projectexplorer/images/settingscategory_cpp.png");
    setWidgetCreator([] { return new CodeGenSettingsPageWidget; });
}

} // namespace Internal
} // namespace QtSupport
