// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "bindingeditordialog.h"

#include <texteditor/texteditor.h>

#include <qmldesigner/qmldesignerplugin.h>
#include <qmljseditor/qmljseditor.h>
#include <qmljseditor/qmljseditordocument.h>
#include <texteditor/textdocument.h>

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QPlainTextEdit>
#include <QPushButton>

namespace QmlDesigner {

BindingEditorDialog::BindingEditorDialog(QWidget *parent)
    : AbstractEditorDialog(parent, tr("Binding Editor"))
{
    setupUIComponents();

    QObject::connect(m_comboBoxItem, &QComboBox::currentIndexChanged,
                     this, &BindingEditorDialog::itemIDChanged);
    QObject::connect(m_comboBoxProperty, &QComboBox::currentIndexChanged,
                     this, &BindingEditorDialog::propertyIDChanged);
    QObject::connect(m_checkBoxNot, &QCheckBox::stateChanged,
                     this, &BindingEditorDialog::checkBoxChanged);
}

BindingEditorDialog::~BindingEditorDialog()
{
}

void BindingEditorDialog::adjustProperties()
{
    QString expression = editorValue().trimmed();

    m_checkBoxNot->setChecked(expression.startsWith("!"));
    if (m_checkBoxNot->isChecked())
        expression.remove(0, 1);

    QString item;
    QString property;
    QStringList expressionElements = expression.split(".");

    if (!expressionElements.isEmpty()) {
        const int itemIndex = m_bindings.indexOf(expressionElements.at(0));

        if (itemIndex != -1) {
            item = expressionElements.at(0);
            expressionElements.removeFirst();

            if (!expressionElements.isEmpty()) {
                const QString sum = expressionElements.join(".");

                if (m_bindings.at(itemIndex).properties.contains(sum))
                    property = sum;
            }
        }
    }

    if (item.isEmpty()) {
        item = undefinedString;
        if (m_comboBoxItem->findText(item) == -1)
            m_comboBoxItem->addItem(item);
    }
    m_comboBoxItem->setCurrentText(item);

    if (property.isEmpty()) {
        property = undefinedString;
        if (m_comboBoxProperty->findText(property) == -1)
            m_comboBoxProperty->addItem(property);
    }
    m_comboBoxProperty->setCurrentText(property);
}

void BindingEditorDialog::setAllBindings(const QList<BindingOption> &bindings, const NodeMetaInfo &type)
{
    m_lock = true;

    m_bindings = bindings;
    m_type = type;
    setupComboBoxes();
    setupCheckBox();
    adjustProperties();

    m_lock = false;
}

void BindingEditorDialog::setupUIComponents()
{
    m_comboBoxItem = new QComboBox(this);
    m_comboBoxProperty = new QComboBox(this);
    m_checkBoxNot = new QCheckBox(this);
    m_checkBoxNot->setText(tr("NOT"));
    m_checkBoxNot->setVisible(false);
    m_checkBoxNot->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
    m_checkBoxNot->setToolTip(tr("Invert the boolean expression."));

    m_comboBoxLayout->addWidget(m_comboBoxItem);
    m_comboBoxLayout->addWidget(m_comboBoxProperty);
    m_comboBoxLayout->addWidget(m_checkBoxNot);
}

void BindingEditorDialog::setupComboBoxes()
{
    m_comboBoxItem->clear();
    m_comboBoxProperty->clear();

    for (const auto &bind : std::as_const(m_bindings))
        m_comboBoxItem->addItem(bind.item);
}

void BindingEditorDialog::setupCheckBox()
{
    const bool visible = m_type.isBool();
    m_checkBoxNot->setVisible(visible);
}

void BindingEditorDialog::itemIDChanged(int itemID)
{
    const QString previousProperty = m_comboBoxProperty->currentText();
    m_comboBoxProperty->clear();

    if (m_bindings.size() > itemID && itemID != -1) {
        m_comboBoxProperty->addItems(m_bindings.at(itemID).properties);

        if (!m_lock)
            if (m_comboBoxProperty->findText(previousProperty) != -1)
                m_comboBoxProperty->setCurrentText(previousProperty);

        const int undefinedItem = m_comboBoxItem->findText(undefinedString);
        if ((undefinedItem != -1) && (m_comboBoxItem->itemText(itemID) != undefinedString))
            m_comboBoxItem->removeItem(undefinedItem);
    }
}

void BindingEditorDialog::propertyIDChanged(int propertyID)
{
    const int itemID = m_comboBoxItem->currentIndex();

    if (!m_lock)
        if (!m_comboBoxProperty->currentText().isEmpty() && (m_comboBoxProperty->currentText() != undefinedString)) {
            QString expression = m_comboBoxItem->itemText(itemID) + "." + m_comboBoxProperty->itemText(propertyID);
            if (m_checkBoxNot->isChecked())
                expression.prepend("!");

            setEditorValue(expression);
        }

    const int undefinedProperty = m_comboBoxProperty->findText(undefinedString);
    if ((undefinedProperty != -1) && (m_comboBoxProperty->itemText(propertyID) != undefinedString))
        m_comboBoxProperty->removeItem(undefinedProperty);
}

void BindingEditorDialog::checkBoxChanged(int state)
{
    if (m_lock)
        return;

    QString expression = editorValue().trimmed();
    if (state == Qt::Checked)
        expression.prepend("!");
    else
        expression.remove(0, 1);

    setEditorValue(expression);
}

} // QmlDesigner namespace
