// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "codegensettingspage.h"

#include "codegensettings.h"
#include "qtsupportconstants.h"
#include "qtsupporttr.h"

#include <coreplugin/icore.h>

#include <cppeditor/cppeditorconstants.h>

#include <utils/layoutbuilder.h>

#include <QCheckBox>
#include <QRadioButton>

namespace QtSupport::Internal {

class CodeGenSettingsPageWidget : public Core::IOptionsPageWidget
{
public:
    CodeGenSettingsPageWidget();

private:
    void apply() final;

    int uiEmbedding() const;

    QRadioButton *m_ptrAggregationRadioButton;
    QRadioButton *m_aggregationButton;
    QRadioButton *m_multipleInheritanceButton;
    QCheckBox *m_retranslateCheckBox;
    QCheckBox *m_includeQtModuleCheckBox;
    QCheckBox *m_addQtVersionCheckBox;
};

CodeGenSettingsPageWidget::CodeGenSettingsPageWidget()
{
    resize(340, 232);

    CodeGenSettings parameters;
    parameters.fromSettings(Core::ICore::settings());

    using namespace Utils::Layouting;

    m_ptrAggregationRadioButton = new QRadioButton(Tr::tr("Aggregation as a pointer member"));
    m_ptrAggregationRadioButton->setChecked
        (parameters.embedding == CodeGenSettings::PointerAggregatedUiClass);

    m_aggregationButton = new QRadioButton(Tr::tr("Aggregation"));
    m_aggregationButton->setChecked
        (parameters.embedding == CodeGenSettings::AggregatedUiClass);

    m_multipleInheritanceButton = new QRadioButton(Tr::tr("Multiple inheritance"));
    m_multipleInheritanceButton->setChecked
        (parameters.embedding == CodeGenSettings::InheritedUiClass);

    m_retranslateCheckBox = new QCheckBox(Tr::tr("Support for changing languages at runtime"));
    m_retranslateCheckBox->setChecked(parameters.retranslationSupport);

    m_includeQtModuleCheckBox = new QCheckBox(Tr::tr("Use Qt module name in #include-directive"));
    m_includeQtModuleCheckBox->setChecked(parameters.includeQtModule);

    m_addQtVersionCheckBox = new QCheckBox(Tr::tr("Add Qt version #ifdef for module names"));
    m_addQtVersionCheckBox->setChecked(parameters.addQtVersionCheck);
    m_addQtVersionCheckBox->setEnabled(false);

    Column {
        Group {
            title(Tr::tr("Embedding of the UI Class")),
            Column {
                m_ptrAggregationRadioButton,
                m_aggregationButton,
                m_multipleInheritanceButton
            }
        },
        Group {
            title(Tr::tr("Code Generation")),
            Column {
                m_retranslateCheckBox,
                m_includeQtModuleCheckBox,
                m_addQtVersionCheckBox
            }
        },
        st
    }.attachTo(this);

    connect(m_includeQtModuleCheckBox, &QAbstractButton::toggled,
            m_addQtVersionCheckBox, &QWidget::setEnabled);
}

void CodeGenSettingsPageWidget::apply()
{
    CodeGenSettings rc;
    rc.embedding = static_cast<CodeGenSettings::UiClassEmbedding>(uiEmbedding());
    rc.retranslationSupport = m_retranslateCheckBox->isChecked();
    rc.includeQtModule = m_includeQtModuleCheckBox->isChecked();
    rc.addQtVersionCheck = m_addQtVersionCheckBox->isChecked();
    rc.toSettings(Core::ICore::settings());
}

int CodeGenSettingsPageWidget::uiEmbedding() const
{
    if (m_ptrAggregationRadioButton->isChecked())
        return CodeGenSettings::PointerAggregatedUiClass;
    if (m_aggregationButton->isChecked())
        return CodeGenSettings::AggregatedUiClass;
    return CodeGenSettings::InheritedUiClass;
}

// ---------- CodeGenSettingsPage

CodeGenSettingsPage::CodeGenSettingsPage()
{
    setId(Constants::CODEGEN_SETTINGS_PAGE_ID);
    setDisplayName(Tr::tr("Qt Class Generation"));
    setCategory(CppEditor::Constants::CPP_SETTINGS_CATEGORY);
    setDisplayCategory(
        QCoreApplication::translate("CppEditor", CppEditor::Constants::CPP_SETTINGS_NAME));
    setCategoryIconPath(":/projectexplorer/images/settingscategory_cpp.png");
    setWidgetCreator([] { return new CodeGenSettingsPageWidget; });
}

} // QtSupport::Internal
