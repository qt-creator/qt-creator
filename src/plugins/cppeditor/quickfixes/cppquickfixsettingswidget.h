// Copyright (C) 2020 Leander Schulten <Leander.Schulten@rwth-aachen.de>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/dialogs/ioptionspage.h>

#include <QApplication>
#include <QRegularExpression>
#include <QWidget>

QT_BEGIN_NAMESPACE
class QCheckBox;
class QGroupBox;
class QLineEdit;
class QListWidget;
class QListWidgetItem;
class QPushButton;
class QRadioButton;
QT_END_NAMESPACE

namespace CppEditor { class CppQuickFixSettings; }

namespace CppEditor::Internal {

class LineCountSpinBox;

class CppQuickFixSettingsWidget : public Core::IOptionsPageWidget
{
    Q_OBJECT

    enum CustomDataRoles {
        Types = Qt::UserRole,
        Comparison,
        Assignment,
        ReturnExpression,
        ReturnType,
    };

public:
    CppQuickFixSettingsWidget();

    void loadSettings(CppQuickFixSettings *settings);
    void saveSettings(CppQuickFixSettings *settings);

signals:
    void settingsChanged();

private:
    void apply() final;
    void currentCustomItemChanged(QListWidgetItem *newItem, QListWidgetItem *oldItem);

    bool m_isLoadingSettings = false;
    const QRegularExpression m_typeSplitter;

    LineCountSpinBox *m_lines_getterOutsideClass;
    LineCountSpinBox *m_lines_getterInCppFile;
    LineCountSpinBox *m_lines_setterOutsideClass;
    LineCountSpinBox *m_lines_setterInCppFile;
    QLineEdit *m_lineEdit_setterParameter;
    QCheckBox *m_checkBox_setterSlots;
    QCheckBox *m_checkBox_signalWithNewValue;
    QLineEdit *m_lineEdit_getterName;
    QLineEdit *m_lineEdit_resetName;
    QLineEdit *m_lineEdit_getterAttribute;
    QLineEdit *m_lineEdit_setterName;
    QLineEdit *m_lineEdit_signalName;
    QLineEdit *m_lineEdit_memberVariableName;
    QRadioButton *m_radioButton_generateMissingNamespace;
    QRadioButton *m_radioButton_addUsingnamespace;
    QRadioButton *m_radioButton_rewriteTypes;
    QCheckBox *m_useAutoCheckBox;
    QGroupBox *m_groupBox_customTemplate;
    QLineEdit *m_lineEdit_customTemplateTypes;
    QLineEdit *m_lineEdit_customTemplateComparison;
    QLineEdit *m_lineEdit_customTemplateAssignment;
    QLineEdit *m_lineEdit_customTemplateReturnExpression;
    QLineEdit *m_lineEdit_customTemplateReturnType;
    QListWidget *m_listWidget_customTemplates;
    QPushButton *m_pushButton_removeCustomTemplate;
    QListWidget *m_valueTypes;
    QCheckBox *m_returnByConstRefCheckBox;
};

} // CppEditor::Internal
