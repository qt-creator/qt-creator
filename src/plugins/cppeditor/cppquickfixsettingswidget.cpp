/****************************************************************************
**
** Copyright (C) 2020 Leander Schulten <Leander.Schulten@rwth-aachen.de>
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

#include "cppquickfixsettingswidget.h"
#include "cppquickfixsettings.h"
#include "ui_cppquickfixsettingswidget.h"
#include <QtDebug>

using namespace CppEditor;
using namespace CppEditor::Internal;

CppQuickFixSettingsWidget::CppQuickFixSettingsWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new ::Ui::CppQuickFixSettingsWidget)
    , typeSplitter("\\s*,\\s*")
{
    ui->setupUi(this);
    QObject::connect(ui->listWidget_customTemplates,
                     &QListWidget::currentItemChanged,
                     this,
                     &CppQuickFixSettingsWidget::currentCustomItemChanged);
    QObject::connect(ui->pushButton_addValueType, &QPushButton::clicked, [this] {
        auto item = new QListWidgetItem("<type>", ui->valueTypes);
        item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsEnabled
                       | Qt::ItemNeverHasChildren);
        ui->valueTypes->scrollToItem(item);
        item->setSelected(true);
    });
    QObject::connect(ui->pushButton_addCustomTemplate, &QPushButton::clicked, [this] {
        auto item = new QListWidgetItem("<type>", ui->listWidget_customTemplates);
        item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemNeverHasChildren);
        ui->listWidget_customTemplates->scrollToItem(item);
        ui->listWidget_customTemplates->setCurrentItem(item);
        ui->lineEdit_customTemplateTypes->setText("<type>");
    });
    QObject::connect(ui->pushButton_removeCustomTemplate, &QPushButton::clicked, [this] {
        delete ui->listWidget_customTemplates->currentItem();
    });
    QObject::connect(ui->pushButton_removeValueType, &QPushButton::clicked, [this] {
        delete ui->valueTypes->currentItem();
    });
    this->setEnabled(false);
    this->ui->pushButton_removeCustomTemplate->setEnabled(false);
    this->ui->groupBox_customTemplate->setEnabled(false);
    this->ui->widget_getterCpp->setEnabled(false);
    this->ui->widget_setterCpp->setEnabled(false);
    this->ui->widget_getterOutside->setEnabled(false);
    this->ui->widget_setterOutside->setEnabled(false);

    const QString toolTip = QCoreApplication::translate("CppQuickFixSettingsWidget",
                                                        R"==(Use <name> for the variable
Use <camel> for camel case
Use <snake> for snake case
Use <Name>, <Camel> and <Snake> for upper case
e.g. name = "m_test_foo_":
"set_<name> => "set_test_foo"
"set<Name> => "setTest_foo"
"set<Camel> => "setTestFoo")==");
    this->ui->lineEdit_resetName->setToolTip(toolTip);
    this->ui->lineEdit_setterName->setToolTip(toolTip);
    this->ui->lineEdit_setterParameter->setToolTip(toolTip);
    this->ui->lineEdit_signalName->setToolTip(toolTip);
    this->ui->lineEdit_getterName->setToolTip(toolTip);
    this->ui->lineEdit_memberVariableName->setToolTip(toolTip);

    // connect controls to settingsChanged signal
    auto then = [this] {
        if (!isLoadingSettings)
            emit settingsChanged();
    };
    QObject::connect(this->ui->checkBox_getterCpp, &QCheckBox::clicked, then);
    QObject::connect(this->ui->checkBox_getterOutside, &QCheckBox::clicked, then);
    QObject::connect(this->ui->checkBox_setterCpp, &QCheckBox::clicked, then);
    QObject::connect(this->ui->checkBox_setterOutside, &QCheckBox::clicked, then);
    QObject::connect(this->ui->checkBox_setterSlots, &QCheckBox::clicked, then);
    QObject::connect(this->ui->checkBox_signalWithNewValue, &QCheckBox::clicked, then);
    QObject::connect(this->ui->pushButton_addCustomTemplate, &QPushButton::clicked, then);
    QObject::connect(this->ui->pushButton_removeCustomTemplate, &QPushButton::clicked, then);
    QObject::connect(this->ui->pushButton_addValueType, &QPushButton::clicked, then);
    QObject::connect(this->ui->pushButton_removeValueType, &QPushButton::clicked, then);
    QObject::connect(this->ui->valueTypes, &QListWidget::itemChanged, then);
    QObject::connect(this->ui->lineEdit_customTemplateAssignment, &QLineEdit::textEdited, then);
    QObject::connect(this->ui->lineEdit_customTemplateComparison, &QLineEdit::textEdited, then);
    QObject::connect(this->ui->lineEdit_customTemplateReturnExpression, &QLineEdit::textEdited, then);
    QObject::connect(this->ui->lineEdit_customTemplateReturnType, &QLineEdit::textEdited, then);
    QObject::connect(this->ui->lineEdit_customTemplateTypes, &QLineEdit::textEdited, then);
    QObject::connect(this->ui->lineEdit_getterAttribute, &QLineEdit::textEdited, then);
    QObject::connect(this->ui->lineEdit_getterName, &QLineEdit::textEdited, then);
    QObject::connect(this->ui->lineEdit_memberVariableName, &QLineEdit::textEdited, then);
    QObject::connect(this->ui->lineEdit_resetName, &QLineEdit::textEdited, then);
    QObject::connect(this->ui->lineEdit_setterName, &QLineEdit::textEdited, then);
    QObject::connect(this->ui->lineEdit_setterParameter, &QLineEdit::textEdited, then);
    QObject::connect(this->ui->lineEdit_signalName, &QLineEdit::textEdited, then);
    QObject::connect(this->ui->spinBox_getterCpp, &QSpinBox::editingFinished, then);
    QObject::connect(this->ui->spinBox_getterOutside, &QSpinBox::editingFinished, then);
    QObject::connect(this->ui->spinBox_setterCpp, &QSpinBox::editingFinished, then);
    QObject::connect(this->ui->spinBox_setterOutside, &QSpinBox::editingFinished, then);
    QObject::connect(this->ui->radioButton_addUsingnamespace, &QRadioButton::clicked, then);
    QObject::connect(this->ui->radioButton_generateMissingNamespace, &QRadioButton::clicked, then);
    QObject::connect(this->ui->radioButton_rewriteTypes, &QRadioButton::clicked, then);
}

CppQuickFixSettingsWidget::~CppQuickFixSettingsWidget()
{
    delete ui;
}

void CppQuickFixSettingsWidget::loadSettings(CppQuickFixSettings *settings)
{
    isLoadingSettings = true;
    ui->checkBox_getterCpp->setChecked(settings->getterInCppFileFrom > 0);
    ui->spinBox_getterCpp->setValue(std::abs(settings->getterInCppFileFrom));
    ui->checkBox_setterCpp->setChecked(settings->setterInCppFileFrom > 0);
    ui->spinBox_setterCpp->setValue(std::abs(settings->setterInCppFileFrom));
    ui->checkBox_getterOutside->setChecked(settings->getterOutsideClassFrom > 0);
    ui->spinBox_getterOutside->setValue(std::abs(settings->getterOutsideClassFrom));
    ui->checkBox_setterOutside->setChecked(settings->setterOutsideClassFrom > 0);
    ui->spinBox_setterOutside->setValue(std::abs(settings->setterOutsideClassFrom));
    ui->lineEdit_getterAttribute->setText(settings->getterAttributes);
    ui->lineEdit_getterName->setText(settings->getterNameTemplate);
    ui->lineEdit_setterName->setText(settings->setterNameTemplate);
    ui->lineEdit_setterParameter->setText(settings->setterParameterNameTemplate);
    switch (settings->cppFileNamespaceHandling) {
    case CppQuickFixSettings::MissingNamespaceHandling::RewriteType:
        ui->radioButton_rewriteTypes->setChecked(true);
        break;
    case CppQuickFixSettings::MissingNamespaceHandling::CreateMissing:
        ui->radioButton_generateMissingNamespace->setChecked(true);
        break;
    case CppQuickFixSettings::MissingNamespaceHandling::AddUsingDirective:
        ui->radioButton_addUsingnamespace->setChecked(true);
        break;
    }
    ui->lineEdit_resetName->setText(settings->resetNameTemplate);
    ui->lineEdit_signalName->setText(settings->signalNameTemplate);
    ui->lineEdit_memberVariableName->setText(settings->memberVariableNameTemplate);
    ui->checkBox_setterSlots->setChecked(settings->setterAsSlot);
    ui->checkBox_signalWithNewValue->setChecked(settings->signalWithNewValue);
    ui->valueTypes->clear();
    for (const auto &valueType : qAsConst(settings->valueTypes)) {
        auto item = new QListWidgetItem(valueType, ui->valueTypes);
        item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsEnabled
                       | Qt::ItemNeverHasChildren);
    }
    ui->listWidget_customTemplates->clear();
    for (const auto &customTemplate : settings->customTemplates) {
        auto item = new QListWidgetItem(customTemplate.types.join(", "),
                                        ui->listWidget_customTemplates);
        item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemNeverHasChildren);
        item->setData(CustomDataRoles::Types, customTemplate.types.join(", "));
        item->setData(CustomDataRoles::Comparison, customTemplate.equalComparison);
        item->setData(CustomDataRoles::Assignment, customTemplate.assignment);
        item->setData(CustomDataRoles::ReturnType, customTemplate.returnType);
        item->setData(CustomDataRoles::ReturnExpression, customTemplate.returnExpression);
    }
    if (ui->listWidget_customTemplates->count() > 0) {
        ui->listWidget_customTemplates->setCurrentItem(ui->listWidget_customTemplates->item(0));
    }
    this->setEnabled(true);
    isLoadingSettings = false;
}

void CppQuickFixSettingsWidget::saveSettings(CppQuickFixSettings *settings)
{
    // first write the current selected custom template back to the model
    if (ui->listWidget_customTemplates->currentItem() != nullptr) {
        auto item = ui->listWidget_customTemplates->currentItem();
        auto list = ui->lineEdit_customTemplateTypes->text().split(typeSplitter, Qt::SkipEmptyParts);
        item->setData(CustomDataRoles::Types, list);
        item->setData(CustomDataRoles::Comparison, ui->lineEdit_customTemplateComparison->text());
        item->setData(CustomDataRoles::Assignment, ui->lineEdit_customTemplateAssignment->text());
        item->setData(CustomDataRoles::ReturnType, ui->lineEdit_customTemplateReturnType->text());
        item->setData(CustomDataRoles::ReturnExpression,
                      ui->lineEdit_customTemplateReturnExpression->text());
    }
    const auto preGetOut = ui->checkBox_getterOutside->isChecked() ? 1 : -1;
    settings->getterOutsideClassFrom = preGetOut * ui->spinBox_getterOutside->value();
    const auto preSetOut = ui->checkBox_setterOutside->isChecked() ? 1 : -1;
    settings->setterOutsideClassFrom = preSetOut * ui->spinBox_setterOutside->value();
    const auto preGetCpp = ui->checkBox_getterCpp->isChecked() ? 1 : -1;
    settings->getterInCppFileFrom = preGetCpp * ui->spinBox_getterCpp->value();
    const auto preSetCpp = ui->checkBox_setterCpp->isChecked() ? 1 : -1;
    settings->setterInCppFileFrom = preSetCpp * ui->spinBox_setterCpp->value();
    settings->setterParameterNameTemplate = ui->lineEdit_setterParameter->text();
    settings->setterAsSlot = ui->checkBox_setterSlots->isChecked();
    settings->signalWithNewValue = ui->checkBox_signalWithNewValue->isChecked();
    settings->getterAttributes = ui->lineEdit_getterAttribute->text();
    settings->getterNameTemplate = ui->lineEdit_getterName->text();
    settings->setterNameTemplate = ui->lineEdit_setterName->text();
    settings->resetNameTemplate = ui->lineEdit_resetName->text();
    settings->signalNameTemplate = ui->lineEdit_signalName->text();
    settings->memberVariableNameTemplate = ui->lineEdit_memberVariableName->text();
    if (ui->radioButton_rewriteTypes->isChecked()) {
        settings->cppFileNamespaceHandling = CppQuickFixSettings::MissingNamespaceHandling::RewriteType;
    } else if (ui->radioButton_addUsingnamespace->isChecked()) {
        settings->cppFileNamespaceHandling = CppQuickFixSettings::MissingNamespaceHandling::AddUsingDirective;
    } else if (ui->radioButton_generateMissingNamespace->isChecked()) {
        settings->cppFileNamespaceHandling = CppQuickFixSettings::MissingNamespaceHandling::CreateMissing;
    }
    settings->valueTypes.clear();
    for (int i = 0; i < ui->valueTypes->count(); ++i) {
        settings->valueTypes << ui->valueTypes->item(i)->text();
    }
    settings->customTemplates.clear();
    for (int i = 0; i < ui->listWidget_customTemplates->count(); ++i) {
        auto item = ui->listWidget_customTemplates->item(i);
        CppQuickFixSettings::CustomTemplate t;
        t.types = item->data(CustomDataRoles::Types).toStringList();
        t.equalComparison = item->data(CustomDataRoles::Comparison).toString();
        t.assignment = item->data(CustomDataRoles::Assignment).toString();
        t.returnExpression = item->data(CustomDataRoles::ReturnExpression).toString();
        t.returnType = item->data(CustomDataRoles::ReturnType).toString();
        settings->customTemplates.push_back(t);
    }
}

void CppQuickFixSettingsWidget::currentCustomItemChanged(QListWidgetItem *newItem,
                                                         QListWidgetItem *oldItem)
{
    if (oldItem) {
        auto list = ui->lineEdit_customTemplateTypes->text().split(typeSplitter, Qt::SkipEmptyParts);
        oldItem->setData(CustomDataRoles::Types, list);
        oldItem->setData(Qt::DisplayRole, list.join(", "));
        oldItem->setData(CustomDataRoles::Comparison, ui->lineEdit_customTemplateComparison->text());
        oldItem->setData(CustomDataRoles::Assignment, ui->lineEdit_customTemplateAssignment->text());
        oldItem->setData(CustomDataRoles::ReturnType, ui->lineEdit_customTemplateReturnType->text());
        oldItem->setData(CustomDataRoles::ReturnExpression,
                         ui->lineEdit_customTemplateReturnExpression->text());
    }
    this->ui->pushButton_removeCustomTemplate->setEnabled(newItem != nullptr);
    this->ui->groupBox_customTemplate->setEnabled(newItem != nullptr);
    if (newItem) {
        this->ui->lineEdit_customTemplateTypes->setText(
            newItem->data(CustomDataRoles::Types).toStringList().join(", "));
        this->ui->lineEdit_customTemplateComparison->setText(
            newItem->data(CustomDataRoles::Comparison).toString());
        this->ui->lineEdit_customTemplateAssignment->setText(
            newItem->data(CustomDataRoles::Assignment).toString());
        this->ui->lineEdit_customTemplateReturnType->setText(
            newItem->data(CustomDataRoles::ReturnType).toString());
        this->ui->lineEdit_customTemplateReturnExpression->setText(
            newItem->data(CustomDataRoles::ReturnExpression).toString());
    } else {
        this->ui->lineEdit_customTemplateTypes->setText("");
        this->ui->lineEdit_customTemplateComparison->setText("");
        this->ui->lineEdit_customTemplateAssignment->setText("");
        this->ui->lineEdit_customTemplateReturnType->setText("");
        this->ui->lineEdit_customTemplateReturnExpression->setText("");
    }
}
