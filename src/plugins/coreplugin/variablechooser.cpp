/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "variablechooser.h"
#include "ui_variablechooser.h"
#include "variablemanager.h"

using namespace Core;

VariableChooser::VariableChooser(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::VariableChooser),
    m_lineEdit(0),
    m_textEdit(0),
    m_plainTextEdit(0)
{
    setAttribute(Qt::WA_DeleteOnClose);
    ui->setupUi(this);
    m_defaultDescription = ui->variableDescription->text();
    ui->variableList->setAttribute(Qt::WA_MacSmallSize);
    ui->variableList->setAttribute(Qt::WA_MacShowFocusRect, false);
    ui->variableDescription->setAttribute(Qt::WA_MacSmallSize);
    setWindowFlags(Qt::Tool | Qt::WindowStaysOnTopHint);
    setFocusProxy(ui->variableList);

    VariableManager *vm = VariableManager::instance();
    foreach (const QString &variable, vm->variables()) {
        ui->variableList->addItem(variable);
    }

    connect(ui->variableList, SIGNAL(currentTextChanged(QString)),
            this, SLOT(updateDescription(QString)));
    connect(ui->variableList, SIGNAL(itemActivated(QListWidgetItem*)),
            this, SLOT(handleItemActivated(QListWidgetItem*)));
    connect(qApp, SIGNAL(focusChanged(QWidget*,QWidget*)),
            this, SLOT(updateCurrentEditor(QWidget*)));
    updateCurrentEditor(qApp->focusWidget());
}

VariableChooser::~VariableChooser()
{
    delete ui;
}

void VariableChooser::updateDescription(const QString &variable)
{
    if (variable.isNull())
        ui->variableDescription->setText(m_defaultDescription);
    else
        ui->variableDescription->setText(VariableManager::instance()->variableDescription(variable));
}

void VariableChooser::updateCurrentEditor(QWidget *widget)
{
    if (QLineEdit *lineEdit = qobject_cast<QLineEdit *>(widget)) {
        m_lineEdit = lineEdit;
        m_textEdit = 0;
        m_plainTextEdit = 0;
    } else if (QTextEdit *textEdit = qobject_cast<QTextEdit *>(widget)) {
        m_lineEdit = 0;
        m_textEdit = textEdit;
        m_plainTextEdit = 0;
    } else if (QPlainTextEdit *plainTextEdit = qobject_cast<QPlainTextEdit *>(widget)) {
        m_lineEdit = 0;
        m_textEdit = 0;
        m_plainTextEdit = plainTextEdit;
    }
}

void VariableChooser::handleItemActivated(QListWidgetItem *item)
{
    if (item)
        insertVariable(item->text());
}

void VariableChooser::insertVariable(const QString &variable)
{
    const QString &text = QLatin1String("${") + variable + QLatin1String("}");
    if (m_lineEdit) {
        m_lineEdit->insert(text);
    } else if (m_textEdit) {
        m_textEdit->insertPlainText(text);
    } else if (m_plainTextEdit) {
        m_plainTextEdit->insertPlainText(text);
    }
}
