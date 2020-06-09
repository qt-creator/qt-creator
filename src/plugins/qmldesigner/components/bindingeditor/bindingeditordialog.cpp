/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include "bindingeditordialog.h"

#include <texteditor/texteditor.h>

#include <qmldesigner/qmldesignerplugin.h>
#include <qmljseditor/qmljseditor.h>
#include <qmljseditor/qmljseditordocument.h>
#include <texteditor/textdocument.h>

#include <QDialogButtonBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QComboBox>
#include <QPlainTextEdit>

namespace QmlDesigner {

BindingEditorDialog::BindingEditorDialog(QWidget *parent, DialogType type)
    : QDialog(parent)
    , m_dialogType(type)
{
    setWindowFlag(Qt::Tool, true);
    setWindowTitle(defaultTitle());
    setModal(false);

    setupJSEditor();
    setupUIComponents();

    QObject::connect(m_buttonBox, &QDialogButtonBox::accepted,
                     this, &BindingEditorDialog::accepted);
    QObject::connect(m_buttonBox, &QDialogButtonBox::rejected,
                     this, &BindingEditorDialog::rejected);
    QObject::connect(m_editorWidget, &BindingEditorWidget::returnKeyClicked,
                     this, &BindingEditorDialog::accepted);

    if (m_dialogType == DialogType::BindingDialog) {
        QObject::connect(m_comboBoxItem, QOverload<int>::of(&QComboBox::currentIndexChanged),
                         this, &BindingEditorDialog::itemIDChanged);
        QObject::connect(m_comboBoxProperty, QOverload<int>::of(&QComboBox::currentIndexChanged),
                         this, &BindingEditorDialog::propertyIDChanged);
        QObject::connect(m_editorWidget, &QPlainTextEdit::textChanged,
                         this, &BindingEditorDialog::textChanged);
    }
}

BindingEditorDialog::~BindingEditorDialog()
{
    delete m_editor; //m_editorWidget is handled by basetexteditor destructor
    delete m_buttonBox;
    delete m_comboBoxItem;
    delete m_comboBoxProperty;
    delete m_comboBoxLayout;
    delete m_verticalLayout;
}

void BindingEditorDialog::showWidget()
{
    this->show();
    this->raise();
    m_editorWidget->setFocus();
}

void BindingEditorDialog::showWidget(int x, int y)
{
    showWidget();
    move(QPoint(x, y));
}

QString BindingEditorDialog::editorValue() const
{
    if (!m_editorWidget)
        return {};

    return m_editorWidget->document()->toPlainText();
}

void BindingEditorDialog::setEditorValue(const QString &text)
{
    if (m_editorWidget)
        m_editorWidget->document()->setPlainText(text);
}

void BindingEditorDialog::setAllBindings(QList<BindingEditorDialog::BindingOption> bindings)
{
    m_lock = true;

    m_bindings = bindings;
    setupComboBoxes();
    adjustProperties();

    m_lock = false;
}

void BindingEditorDialog::adjustProperties()
{
    const QString expression = editorValue();
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

void BindingEditorDialog::unregisterAutoCompletion()
{
    if (m_editorWidget)
        m_editorWidget->unregisterAutoCompletion();
}

QString BindingEditorDialog::defaultTitle() const
{
    return titleString;
}

void BindingEditorDialog::setupJSEditor()
{
    static BindingEditorFactory f;
    m_editor = qobject_cast<TextEditor::BaseTextEditor*>(f.createEditor());
    m_editorWidget = qobject_cast<BindingEditorWidget*>(m_editor->editorWidget());

    Core::Context context = m_editor->context();
    context.prepend(BINDINGEDITOR_CONTEXT_ID);
    m_editorWidget->m_context->setContext(context);

    auto qmlDesignerEditor = QmlDesignerPlugin::instance()->currentDesignDocument()->textEditor();

    m_editorWidget->qmljsdocument = qobject_cast<QmlJSEditor::QmlJSEditorWidget *>(
                qmlDesignerEditor->widget())->qmlJsEditorDocument();


    m_editorWidget->setLineNumbersVisible(false);
    m_editorWidget->setMarksVisible(false);
    m_editorWidget->setCodeFoldingSupported(false);
    m_editorWidget->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_editorWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_editorWidget->setTabChangesFocus(true);
}

void BindingEditorDialog::setupUIComponents()
{
    m_verticalLayout = new QVBoxLayout(this);

    if (m_dialogType == DialogType::BindingDialog) {
        m_comboBoxLayout = new QHBoxLayout;
        m_comboBoxItem = new QComboBox(this);
        m_comboBoxProperty = new QComboBox(this);
    }

    m_editorWidget->setParent(this);
    m_editorWidget->setFrameStyle(QFrame::StyledPanel | QFrame::Raised);
    m_editorWidget->show();

    m_buttonBox = new QDialogButtonBox(this);
    m_buttonBox->setOrientation(Qt::Horizontal);
    m_buttonBox->setStandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    m_buttonBox->button(QDialogButtonBox::Ok)->setDefault(true);

    if (m_dialogType == DialogType::BindingDialog) {
        m_comboBoxLayout->addWidget(m_comboBoxItem);
        m_comboBoxLayout->addWidget(m_comboBoxProperty);
        m_verticalLayout->addLayout(m_comboBoxLayout);
    }
    m_verticalLayout->addWidget(m_editorWidget);
    m_verticalLayout->addWidget(m_buttonBox);

    this->resize(660, 240);
}

void BindingEditorDialog::setupComboBoxes()
{
    m_comboBoxItem->clear();
    m_comboBoxProperty->clear();

    for (const auto &bind : m_bindings)
        m_comboBoxItem->addItem(bind.item);
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
        if (!m_comboBoxProperty->currentText().isEmpty() && (m_comboBoxProperty->currentText() != undefinedString))
            setEditorValue(m_comboBoxItem->itemText(itemID) + "." + m_comboBoxProperty->itemText(propertyID));

    const int undefinedProperty = m_comboBoxProperty->findText(undefinedString);
    if ((undefinedProperty != -1) && (m_comboBoxProperty->itemText(propertyID) != undefinedString))
        m_comboBoxProperty->removeItem(undefinedProperty);
}

void BindingEditorDialog::textChanged()
{
    if (m_lock)
        return;

    m_lock = true;
    adjustProperties();
    m_lock = false;
}

} // QmlDesigner namespace
