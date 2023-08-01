// Copyright (C) 2020 Leander Schulten <Leander.Schulten@rwth-aachen.de>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cppquickfixsettingswidget.h"

#include "cppeditortr.h"
#include "cppquickfixsettings.h"

#include <utils/layoutbuilder.h>

#include <QBoxLayout>
#include <QCheckBox>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QRadioButton>
#include <QSpacerItem>
#include <QSpinBox>
#include <QtDebug>

namespace CppEditor::Internal {

class LineCountSpinBox : public QWidget
{
    Q_OBJECT

public:
    LineCountSpinBox(QWidget *parent = nullptr);

    int count() const;
    void setCount(int count);

signals:
    void changed();

private:
    void updateFields();

    QCheckBox *m_checkBox;
    QLabel *m_opLabel;
    QSpinBox *m_spinBox;
    QLabel *m_unitLabel;
};

LineCountSpinBox::LineCountSpinBox(QWidget *parent)
    : QWidget(parent)
{
    m_checkBox = new QCheckBox;
    m_opLabel = new QLabel(Tr::tr("\342\211\245"));
    m_spinBox = new QSpinBox;
    m_spinBox->setMinimum(1);
    m_unitLabel = new QLabel(Tr::tr("lines"));

    using namespace Layouting;
    Row { m_checkBox, m_opLabel, m_spinBox, m_unitLabel, noMargin }.attachTo(this);

    auto handleChange = [this] {
        updateFields();
        emit changed();
    };
    connect(m_checkBox, &QCheckBox::toggled, handleChange);
    connect(m_spinBox, &QSpinBox::valueChanged, handleChange);

    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
}

int LineCountSpinBox::count() const
{
    return m_spinBox->value() * (m_checkBox->isChecked() ? 1 : -1);
}

void LineCountSpinBox::setCount(int count)
{
    m_spinBox->setValue(std::abs(count));
    m_checkBox->setChecked(count > 0);
    updateFields();
}

void LineCountSpinBox::updateFields()
{
    const bool enabled = m_checkBox->isChecked();
    m_opLabel->setEnabled(enabled);
    m_spinBox->setEnabled(enabled);
    m_unitLabel->setEnabled(enabled);
}

CppQuickFixSettingsWidget::CppQuickFixSettingsWidget()
    : m_typeSplitter("\\s*,\\s*")
{
    m_lines_getterOutsideClass = new LineCountSpinBox;
    m_lines_getterInCppFile = new LineCountSpinBox;
    m_lines_setterOutsideClass = new LineCountSpinBox;
    m_lines_setterInCppFile = new LineCountSpinBox;
    auto functionLocationsGrid = new QWidget;
    auto ulLabel = [] (const QString &text) {
        QLabel *label = new QLabel(text);
        QFont font = label->font();
        font.setUnderline(true);
        label->setFont(font);
        return label;
    };

    const QString placeHolderTect = Tr::tr("See tool tip for more information");
    const QString toolTip = Tr::tr(
R"==(Use <name> for the variable
Use <camel> for camel case
Use <snake> for snake case
Use <Name>, <Camel> and <Snake> for upper case
e.g. name = "m_test_foo_":
"set_<name> => "set_test_foo"
"set<Name> => "setTest_foo"
"set<Camel> => "setTestFoo")==");

    m_lineEdit_getterAttribute = new QLineEdit;
    m_lineEdit_getterAttribute->setPlaceholderText(Tr::tr("For example, [[nodiscard]]"));
    m_lineEdit_getterName = new QLineEdit;
    m_lineEdit_getterName->setPlaceholderText(placeHolderTect);
    m_lineEdit_getterName->setToolTip(toolTip);
    m_lineEdit_setterName = new QLineEdit;
    m_lineEdit_setterName->setPlaceholderText(placeHolderTect);
    m_lineEdit_setterName->setToolTip(toolTip);
    m_lineEdit_setterParameter = new QLineEdit;
    m_lineEdit_setterParameter->setPlaceholderText(Tr::tr("For example, new<Name>"));
    m_lineEdit_setterParameter->setToolTip(toolTip);
    m_checkBox_setterSlots = new QCheckBox(Tr::tr("Setters should be slots"));
    m_lineEdit_resetName = new QLineEdit;
    m_lineEdit_resetName->setPlaceholderText(Tr::tr("Normally reset<Name>"));
    m_lineEdit_resetName->setToolTip(toolTip);
    m_lineEdit_signalName = new QLineEdit;
    m_lineEdit_signalName->setPlaceholderText(Tr::tr("Normally <name>Changed"));
    m_lineEdit_signalName->setToolTip(toolTip);
    m_checkBox_signalWithNewValue = new QCheckBox(
                Tr::tr("Generate signals with the new value as parameter"));
    m_lineEdit_memberVariableName = new QLineEdit;
    m_lineEdit_memberVariableName->setPlaceholderText(Tr::tr("For example, m_<name>"));
    m_lineEdit_memberVariableName->setToolTip(toolTip);

    m_radioButton_generateMissingNamespace = new QRadioButton(Tr::tr("Generate missing namespaces"));
    m_radioButton_addUsingnamespace = new QRadioButton(Tr::tr("Add \"using namespace ...\""));
    m_radioButton_rewriteTypes = new QRadioButton(
                Tr::tr("Rewrite types to match the existing namespaces"));

    m_useAutoCheckBox = new QCheckBox(this);
    m_useAutoCheckBox->setToolTip(Tr::tr("<html><head/><body><p>Uncheck this to make Qt Creator try to "
                                         "derive the type of expression in the &quot;Assign to Local "
                                         "Variable&quot; quickfix.</p><p>Note that this might fail for "
                                         "more complex types.</p></body></html>"));
    m_useAutoCheckBox->setText(Tr::tr("Use type \"auto\" when creating new variables"));

    m_groupBox_customTemplate = new QGroupBox(Tr::tr("Template"));
    m_groupBox_customTemplate->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);
    m_groupBox_customTemplate->setEnabled(false);
    m_listWidget_customTemplates = new QListWidget;
    m_listWidget_customTemplates->setMaximumWidth(200);
    m_listWidget_customTemplates->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);

    m_lineEdit_customTemplateTypes = new QLineEdit;
    m_lineEdit_customTemplateTypes->setToolTip(Tr::tr("Separate the types by comma."));
    m_lineEdit_customTemplateComparison = new QLineEdit;
    m_lineEdit_customTemplateAssignment = new QLineEdit;
    m_lineEdit_customTemplateReturnExpression = new QLineEdit;
    m_lineEdit_customTemplateReturnType = new QLineEdit;
    auto customTemplateLabel = new QLabel(Tr::tr("Use <new> and <cur> to access the parameter and "
                                                 "current value. Use <type> to access the type and <T> "
                                                 "for the template parameter."));
    customTemplateLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    customTemplateLabel->setWordWrap(true);

    auto pushButton_addCustomTemplate = new QPushButton;
    pushButton_addCustomTemplate->setText(Tr::tr("Add"));
    m_pushButton_removeCustomTemplate = new QPushButton(Tr::tr("Remove"));
    m_pushButton_removeCustomTemplate->setEnabled(false);

    m_valueTypes = new QListWidget(this);
    m_valueTypes->setToolTip(Tr::tr("Normally arguments get passed by const reference. If the Type is "
                                    "one of the following ones, the argument gets passed by value. "
                                    "Namespaces and template arguments are removed. The real Type must "
                                    "contain the given Type. For example, \"int\" matches \"int32_t\" "
                                    "but not \"vector<int>\". \"vector\" matches "
                                    "\"std::pmr::vector<int>\" but not "
                                    "\"std::optional<vector<int>>\""));
    auto pushButton_addValueType = new QPushButton(Tr::tr("Add"));
    auto pushButton_removeValueType = new QPushButton(Tr::tr("Remove"));

    m_returnByConstRefCheckBox = new QCheckBox(Tr::tr("Return non-value types by const reference"));
    m_returnByConstRefCheckBox->setChecked(false);

    connect(m_listWidget_customTemplates, &QListWidget::currentItemChanged,
            this, &CppQuickFixSettingsWidget::currentCustomItemChanged);

    connect(pushButton_addValueType, &QPushButton::clicked, this, [this] {
        auto item = new QListWidgetItem("<type>", m_valueTypes);
        item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsEnabled
                       | Qt::ItemNeverHasChildren);
        m_valueTypes->scrollToItem(item);
        item->setSelected(true);
    });
    connect(pushButton_addCustomTemplate, &QPushButton::clicked, this, [this] {
        auto item = new QListWidgetItem("<type>", m_listWidget_customTemplates);
        item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemNeverHasChildren);
        m_listWidget_customTemplates->scrollToItem(item);
        m_listWidget_customTemplates->setCurrentItem(item);
        m_lineEdit_customTemplateTypes->setText("<type>");
    });
    connect(m_pushButton_removeCustomTemplate, &QPushButton::clicked, this, [this] {
        delete m_listWidget_customTemplates->currentItem();
    });
    connect(pushButton_removeValueType, &QPushButton::clicked, this, [this] {
        delete m_valueTypes->currentItem();
    });

    setEnabled(false);

    using namespace Layouting;

    Grid {
        empty, ulLabel(Tr::tr("Generate Setters")), ulLabel(Tr::tr("Generate Getters")), br,
        Tr::tr("Inside class:"), Tr::tr("Default"), Tr::tr("Default"), br,
        Tr::tr("Outside class:"), m_lines_setterOutsideClass, m_lines_getterOutsideClass, br,
        Tr::tr("In .cpp file:"), m_lines_setterInCppFile, m_lines_getterInCppFile, br,
        noMargin,
    }.attachTo(functionLocationsGrid);

    if (QGridLayout *gl = qobject_cast<QGridLayout*>(functionLocationsGrid->layout()))
        gl->setHorizontalSpacing(48);

    Form {
        Tr::tr("Types:"), m_lineEdit_customTemplateTypes, br,
        Tr::tr("Comparison:"), m_lineEdit_customTemplateComparison, br,
        Tr::tr("Assignment:"), m_lineEdit_customTemplateAssignment, br,
        Tr::tr("Return expression:"), m_lineEdit_customTemplateReturnExpression, br,
        Tr::tr("Return type:"), m_lineEdit_customTemplateReturnType, br,
        customTemplateLabel, br,
    }.attachTo(m_groupBox_customTemplate);

    Column {
        Group {
            title(Tr::tr("Generated Function Locations")),
            Row { functionLocationsGrid, st, },
        },
        Group {
            title(Tr::tr("Getter Setter Generation Properties")),
            Form {
                Tr::tr("Getter attributes:"), m_lineEdit_getterAttribute, br,
                Tr::tr("Getter name:"), m_lineEdit_getterName, br,
                Tr::tr("Setter name:"), m_lineEdit_setterName, br,
                Tr::tr("Setter parameter name:"), m_lineEdit_setterParameter, br,
                m_checkBox_setterSlots, br,
                Tr::tr("Reset name:"), m_lineEdit_resetName, br,
                Tr::tr("Signal name:"), m_lineEdit_signalName, br,
                m_checkBox_signalWithNewValue, br,
                Tr::tr("Member variable name:"), m_lineEdit_memberVariableName, br,
            },
        },
        Group {
            title(Tr::tr("Missing Namespace Handling")),
            Form {
                m_radioButton_generateMissingNamespace, br,
                m_radioButton_addUsingnamespace, br,
                m_radioButton_rewriteTypes, br,
            },
        },
        m_useAutoCheckBox,
        Group {
            title(Tr::tr("Custom Getter Setter Templates")),
            Row {
                Column {
                    m_listWidget_customTemplates,
                    Row { pushButton_addCustomTemplate, m_pushButton_removeCustomTemplate, },
                },
                m_groupBox_customTemplate,
            },
        },
        Group {
            title(Tr::tr("Value types:")),
            Row {
                m_valueTypes,
                Column { pushButton_addValueType, pushButton_removeValueType, st, },
            },
        },
        m_returnByConstRefCheckBox,
    }.attachTo(this);

    // connect controls to settingsChanged signal
    auto then = [this] {
        if (!m_isLoadingSettings)
            emit settingsChanged();
    };

    connect(m_lines_setterOutsideClass, &LineCountSpinBox::changed, then);
    connect(m_lines_setterInCppFile, &LineCountSpinBox::changed, then);
    connect(m_lines_getterOutsideClass, &LineCountSpinBox::changed, then);
    connect(m_lines_getterInCppFile, &LineCountSpinBox::changed, then);
    connect(m_checkBox_setterSlots, &QCheckBox::clicked, then);
    connect(m_checkBox_signalWithNewValue, &QCheckBox::clicked, then);
    connect(pushButton_addCustomTemplate, &QPushButton::clicked, then);
    connect(m_pushButton_removeCustomTemplate, &QPushButton::clicked, then);
    connect(pushButton_addValueType, &QPushButton::clicked, then);
    connect(pushButton_removeValueType, &QPushButton::clicked, then);
    connect(m_useAutoCheckBox, &QCheckBox::clicked, then);
    connect(m_valueTypes, &QListWidget::itemChanged, then);
    connect(m_returnByConstRefCheckBox, &QCheckBox::clicked, then);
    connect(m_lineEdit_customTemplateAssignment, &QLineEdit::textEdited, then);
    connect(m_lineEdit_customTemplateComparison, &QLineEdit::textEdited, then);
    connect(m_lineEdit_customTemplateReturnExpression, &QLineEdit::textEdited, then);
    connect(m_lineEdit_customTemplateReturnType, &QLineEdit::textEdited, then);
    connect(m_lineEdit_customTemplateTypes, &QLineEdit::textEdited, then);
    connect(m_lineEdit_getterAttribute, &QLineEdit::textEdited, then);
    connect(m_lineEdit_getterName, &QLineEdit::textEdited, then);
    connect(m_lineEdit_memberVariableName, &QLineEdit::textEdited, then);
    connect(m_lineEdit_resetName, &QLineEdit::textEdited, then);
    connect(m_lineEdit_setterName, &QLineEdit::textEdited, then);
    connect(m_lineEdit_setterParameter, &QLineEdit::textEdited, then);
    connect(m_lineEdit_signalName, &QLineEdit::textEdited, then);
    connect(m_radioButton_addUsingnamespace, &QRadioButton::clicked, then);
    connect(m_radioButton_generateMissingNamespace, &QRadioButton::clicked, then);
    connect(m_radioButton_rewriteTypes, &QRadioButton::clicked, then);

    loadSettings(CppQuickFixSettings::instance());
}

void CppQuickFixSettingsWidget::loadSettings(CppQuickFixSettings *settings)
{
    m_isLoadingSettings = true;
    m_lines_getterOutsideClass->setCount(settings->getterOutsideClassFrom);
    m_lines_getterInCppFile->setCount(settings->getterInCppFileFrom);
    m_lines_setterOutsideClass->setCount(settings->setterOutsideClassFrom);
    m_lines_setterInCppFile->setCount(settings->setterInCppFileFrom);
    m_lineEdit_getterAttribute->setText(settings->getterAttributes);
    m_lineEdit_getterName->setText(settings->getterNameTemplate);
    m_lineEdit_setterName->setText(settings->setterNameTemplate);
    m_lineEdit_setterParameter->setText(settings->setterParameterNameTemplate);
    switch (settings->cppFileNamespaceHandling) {
    case CppQuickFixSettings::MissingNamespaceHandling::RewriteType:
        m_radioButton_rewriteTypes->setChecked(true);
        break;
    case CppQuickFixSettings::MissingNamespaceHandling::CreateMissing:
        m_radioButton_generateMissingNamespace->setChecked(true);
        break;
    case CppQuickFixSettings::MissingNamespaceHandling::AddUsingDirective:
        m_radioButton_addUsingnamespace->setChecked(true);
        break;
    }
    m_lineEdit_resetName->setText(settings->resetNameTemplate);
    m_lineEdit_signalName->setText(settings->signalNameTemplate);
    m_lineEdit_memberVariableName->setText(settings->memberVariableNameTemplate);
    m_checkBox_setterSlots->setChecked(settings->setterAsSlot);
    m_checkBox_signalWithNewValue->setChecked(settings->signalWithNewValue);
    m_useAutoCheckBox->setChecked(settings->useAuto);
    m_valueTypes->clear();
    for (const auto &valueType : std::as_const(settings->valueTypes)) {
        auto item = new QListWidgetItem(valueType, m_valueTypes);
        item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsEnabled
                       | Qt::ItemNeverHasChildren);
    }
    m_returnByConstRefCheckBox->setChecked(settings->returnByConstRef);
    m_listWidget_customTemplates->clear();
    for (const auto &customTemplate : settings->customTemplates) {
        auto item = new QListWidgetItem(customTemplate.types.join(", "),
                                        m_listWidget_customTemplates);
        item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemNeverHasChildren);
        item->setData(CustomDataRoles::Types, customTemplate.types.join(", "));
        item->setData(CustomDataRoles::Comparison, customTemplate.equalComparison);
        item->setData(CustomDataRoles::Assignment, customTemplate.assignment);
        item->setData(CustomDataRoles::ReturnType, customTemplate.returnType);
        item->setData(CustomDataRoles::ReturnExpression, customTemplate.returnExpression);
    }
    if (m_listWidget_customTemplates->count() > 0) {
        m_listWidget_customTemplates->setCurrentItem(m_listWidget_customTemplates->item(0));
    }
    this->setEnabled(true);
    m_isLoadingSettings = false;
}

void CppQuickFixSettingsWidget::saveSettings(CppQuickFixSettings *settings)
{
    // first write the current selected custom template back to the model
    if (m_listWidget_customTemplates->currentItem() != nullptr) {
        auto item = m_listWidget_customTemplates->currentItem();
        auto list = m_lineEdit_customTemplateTypes->text().split(m_typeSplitter, Qt::SkipEmptyParts);
        item->setData(CustomDataRoles::Types, list);
        item->setData(CustomDataRoles::Comparison, m_lineEdit_customTemplateComparison->text());
        item->setData(CustomDataRoles::Assignment, m_lineEdit_customTemplateAssignment->text());
        item->setData(CustomDataRoles::ReturnType, m_lineEdit_customTemplateReturnType->text());
        item->setData(CustomDataRoles::ReturnExpression,
                      m_lineEdit_customTemplateReturnExpression->text());
    }
    settings->getterOutsideClassFrom = m_lines_getterOutsideClass->count();
    settings->getterInCppFileFrom = m_lines_getterInCppFile->count();
    settings->setterOutsideClassFrom = m_lines_setterOutsideClass->count();
    settings->setterInCppFileFrom = m_lines_setterInCppFile->count();
    settings->setterParameterNameTemplate = m_lineEdit_setterParameter->text();
    settings->setterAsSlot = m_checkBox_setterSlots->isChecked();
    settings->signalWithNewValue = m_checkBox_signalWithNewValue->isChecked();
    settings->getterAttributes = m_lineEdit_getterAttribute->text();
    settings->getterNameTemplate = m_lineEdit_getterName->text();
    settings->setterNameTemplate = m_lineEdit_setterName->text();
    settings->resetNameTemplate = m_lineEdit_resetName->text();
    settings->signalNameTemplate = m_lineEdit_signalName->text();
    settings->memberVariableNameTemplate = m_lineEdit_memberVariableName->text();
    if (m_radioButton_rewriteTypes->isChecked()) {
        settings->cppFileNamespaceHandling = CppQuickFixSettings::MissingNamespaceHandling::RewriteType;
    } else if (m_radioButton_addUsingnamespace->isChecked()) {
        settings->cppFileNamespaceHandling = CppQuickFixSettings::MissingNamespaceHandling::AddUsingDirective;
    } else if (m_radioButton_generateMissingNamespace->isChecked()) {
        settings->cppFileNamespaceHandling = CppQuickFixSettings::MissingNamespaceHandling::CreateMissing;
    }
    settings->useAuto = m_useAutoCheckBox->isChecked();
    settings->valueTypes.clear();
    for (int i = 0; i < m_valueTypes->count(); ++i) {
        settings->valueTypes << m_valueTypes->item(i)->text();
    }
    settings->returnByConstRef = m_returnByConstRefCheckBox->isChecked();
    settings->customTemplates.clear();
    for (int i = 0; i < m_listWidget_customTemplates->count(); ++i) {
        auto item = m_listWidget_customTemplates->item(i);
        CppQuickFixSettings::CustomTemplate t;
        t.types = item->data(CustomDataRoles::Types).toStringList();
        t.equalComparison = item->data(CustomDataRoles::Comparison).toString();
        t.assignment = item->data(CustomDataRoles::Assignment).toString();
        t.returnExpression = item->data(CustomDataRoles::ReturnExpression).toString();
        t.returnType = item->data(CustomDataRoles::ReturnType).toString();
        settings->customTemplates.push_back(t);
    }
}

void CppQuickFixSettingsWidget::apply()
{
    const auto s = CppQuickFixSettings::instance();
    saveSettings(s);
    s->saveAsGlobalSettings();
}

void CppQuickFixSettingsWidget::currentCustomItemChanged(QListWidgetItem *newItem,
                                                         QListWidgetItem *oldItem)
{
    if (oldItem) {
        auto list = m_lineEdit_customTemplateTypes->text().split(m_typeSplitter, Qt::SkipEmptyParts);
        oldItem->setData(CustomDataRoles::Types, list);
        oldItem->setData(Qt::DisplayRole, list.join(", "));
        oldItem->setData(CustomDataRoles::Comparison, m_lineEdit_customTemplateComparison->text());
        oldItem->setData(CustomDataRoles::Assignment, m_lineEdit_customTemplateAssignment->text());
        oldItem->setData(CustomDataRoles::ReturnType, m_lineEdit_customTemplateReturnType->text());
        oldItem->setData(CustomDataRoles::ReturnExpression,
                         m_lineEdit_customTemplateReturnExpression->text());
    }
    m_pushButton_removeCustomTemplate->setEnabled(newItem != nullptr);
    m_groupBox_customTemplate->setEnabled(newItem != nullptr);
    if (newItem) {
        m_lineEdit_customTemplateTypes->setText(
            newItem->data(CustomDataRoles::Types).toStringList().join(", "));
        m_lineEdit_customTemplateComparison->setText(
            newItem->data(CustomDataRoles::Comparison).toString());
        m_lineEdit_customTemplateAssignment->setText(
            newItem->data(CustomDataRoles::Assignment).toString());
        m_lineEdit_customTemplateReturnType->setText(
            newItem->data(CustomDataRoles::ReturnType).toString());
        m_lineEdit_customTemplateReturnExpression->setText(
            newItem->data(CustomDataRoles::ReturnExpression).toString());
    } else {
        m_lineEdit_customTemplateTypes->setText("");
        m_lineEdit_customTemplateComparison->setText("");
        m_lineEdit_customTemplateAssignment->setText("");
        m_lineEdit_customTemplateReturnType->setText("");
        m_lineEdit_customTemplateReturnExpression->setText("");
    }
}

} // CppEditor::Internal

 #include "cppquickfixsettingswidget.moc"
