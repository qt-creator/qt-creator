/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#include "abstracteditordialog.h"

#include <texteditor/texteditor.h>

#include <qmldesigner/qmldesignerplugin.h>
#include <qmljseditor/qmljseditor.h>
#include <qmljseditor/qmljseditordocument.h>
#include <texteditor/textdocument.h>

#include <QDialogButtonBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPlainTextEdit>

namespace QmlDesigner {

AbstractEditorDialog::AbstractEditorDialog(QWidget *parent, const QString &title)
    : QDialog(parent)
    , m_titleString(title)
{
    setWindowFlag(Qt::Tool, true);
    setWindowTitle(defaultTitle());
    setModal(true);

    setupJSEditor();
    setupUIComponents();

    QObject::connect(m_buttonBox, &QDialogButtonBox::accepted,
                     this, &AbstractEditorDialog::accepted);
    QObject::connect(m_buttonBox, &QDialogButtonBox::rejected,
                     this, &AbstractEditorDialog::rejected);
    QObject::connect(m_editorWidget, &BindingEditorWidget::returnKeyClicked,
                     this, &AbstractEditorDialog::accepted);
    QObject::connect(m_editorWidget, &QPlainTextEdit::textChanged,
                     this, &AbstractEditorDialog::textChanged);
}

AbstractEditorDialog::~AbstractEditorDialog()
{
    delete m_editor; // m_editorWidget is handled by basetexteditor destructor
    delete m_buttonBox;
    delete m_comboBoxLayout;
    delete m_verticalLayout;
}

void AbstractEditorDialog::showWidget()
{
    this->show();
    this->raise();
    m_editorWidget->setFocus();
}

void AbstractEditorDialog::showWidget(int x, int y)
{
    showWidget();
    move(QPoint(x, y));
}

QString AbstractEditorDialog::editorValue() const
{
    if (!m_editorWidget)
        return {};

    return m_editorWidget->document()->toPlainText();
}

void AbstractEditorDialog::setEditorValue(const QString &text)
{
    if (m_editorWidget)
        m_editorWidget->document()->setPlainText(text);
}

void AbstractEditorDialog::unregisterAutoCompletion()
{
    if (m_editorWidget)
        m_editorWidget->unregisterAutoCompletion();
}

QString AbstractEditorDialog::defaultTitle() const
{
    return m_titleString;
}

void AbstractEditorDialog::setupJSEditor()
{
    static BindingEditorFactory f;
    m_editor = qobject_cast<TextEditor::BaseTextEditor*>(f.createEditor());
    Q_ASSERT(m_editor);

    m_editorWidget = qobject_cast<BindingEditorWidget*>(m_editor->editorWidget());
    Q_ASSERT(m_editorWidget);

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

void AbstractEditorDialog::setupUIComponents()
{
    m_verticalLayout = new QVBoxLayout(this);

    m_comboBoxLayout = new QHBoxLayout;

    m_editorWidget->setParent(this);
    m_editorWidget->setFrameStyle(QFrame::StyledPanel | QFrame::Raised);
    m_editorWidget->show();

    m_buttonBox = new QDialogButtonBox(this);
    m_buttonBox->setOrientation(Qt::Horizontal);
    m_buttonBox->setStandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    m_buttonBox->button(QDialogButtonBox::Ok)->setDefault(true);

    m_verticalLayout->addLayout(m_comboBoxLayout);
    m_verticalLayout->addWidget(m_editorWidget);
    m_verticalLayout->addWidget(m_buttonBox);

    this->resize(660, 240);
}

bool AbstractEditorDialog::isNumeric(const TypeName &type)
{
    static QList<TypeName> numericTypes = {"double", "int", "real"};
    return numericTypes.contains(type);
}

bool AbstractEditorDialog::isColor(const TypeName &type)
{
    static QList<TypeName> colorTypes = {"QColor", "color"};
    return colorTypes.contains(type);
}

bool AbstractEditorDialog::isVariant(const TypeName &type)
{
    static QList<TypeName> variantTypes = {"alias", "unknown", "variant", "var"};
    return variantTypes.contains(type);
}

void AbstractEditorDialog::textChanged()
{
    if (m_lock)
        return;

    m_lock = true;
    adjustProperties();
    m_lock = false;
}

} // QmlDesigner namespace
