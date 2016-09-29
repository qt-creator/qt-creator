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

#include "qmljscomponentnamedialog.h"
#include "ui_qmljscomponentnamedialog.h"

#include <QFileInfo>
#include <QFileDialog>
#include <QPushButton>

using namespace QmlJSEditor::Internal;

ComponentNameDialog::ComponentNameDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ComponentNameDialog)
{
    ui->setupUi(this);

    connect(ui->pathEdit, &Utils::PathChooser::rawPathChanged,
            this, &ComponentNameDialog::validate);
    connect(ui->componentNameEdit, &QLineEdit::textChanged,
            this, &ComponentNameDialog::validate);
}

ComponentNameDialog::~ComponentNameDialog()
{
    delete ui;
}

bool ComponentNameDialog::go(QString *proposedName,
                             QString *proposedPath,
                             QString *proposedSuffix,
                             const QStringList &properties,
                             const QStringList &sourcePreview,
                             const QString &oldFileName,
                             QStringList *result,
                             QWidget *parent)
{
    Q_ASSERT(proposedName);
    Q_ASSERT(proposedPath);

    const bool isUiFile = QFileInfo(oldFileName).completeSuffix() == "ui.qml";

    ComponentNameDialog d(parent);
    d.ui->componentNameEdit->setNamespacesEnabled(false);
    d.ui->componentNameEdit->setLowerCaseFileName(false);
    d.ui->componentNameEdit->setForceFirstCapitalLetter(true);
    if (proposedName->isEmpty())
        *proposedName = QLatin1String("MyComponent");
    d.ui->componentNameEdit->setText(*proposedName);
    d.ui->pathEdit->setExpectedKind(Utils::PathChooser::ExistingDirectory);
    d.ui->pathEdit->setHistoryCompleter(QLatin1String("QmlJs.Component.History"));
    d.ui->pathEdit->setPath(*proposedPath);
    d.ui->label->setText(tr("Property assignments for %1:").arg(oldFileName));
    d.ui->checkBox->setChecked(isUiFile);
    d.ui->checkBox->setVisible(isUiFile);
    d.m_sourcePreview = sourcePreview;

    d.setProperties(properties);

    d.generateCodePreview();

    d.connect(d.ui->listWidget, &QListWidget::itemChanged, &d, &ComponentNameDialog::generateCodePreview);
    d.connect(d.ui->componentNameEdit, &QLineEdit::textChanged, &d, &ComponentNameDialog::generateCodePreview);

    if (QDialog::Accepted == d.exec()) {
        *proposedName = d.ui->componentNameEdit->text();
        *proposedPath = d.ui->pathEdit->path();

        if (d.ui->checkBox->isChecked())
            *proposedSuffix = "ui.qml";
        else
            *proposedSuffix = "qml";

        if (result)
            *result = d.propertiesToKeep();
        return true;
    }

    return false;
}

void ComponentNameDialog::setProperties(const QStringList &properties)
{
    ui->listWidget->addItems(properties);

    for (int i = 0; i < ui->listWidget->count(); ++i) {
        QListWidgetItem *item = ui->listWidget->item(i);
        item->setFlags(Qt::ItemIsUserCheckable | Qt:: ItemIsEnabled);
        if (item->text() == QLatin1String("x")
                || item->text() == QLatin1String("y"))
            ui->listWidget->item(i)->setCheckState(Qt::Checked);
        else
            ui->listWidget->item(i)->setCheckState(Qt::Unchecked);
    }
}

QStringList ComponentNameDialog::propertiesToKeep() const
{
    QStringList result;
    for (int i = 0; i < ui->listWidget->count(); ++i) {
        QListWidgetItem *item = ui->listWidget->item(i);

        if (item->checkState() == Qt::Checked)
            result.append(item->text());
    }

    return result;
}

void ComponentNameDialog::generateCodePreview()
{
     const QString componentName = ui->componentNameEdit->text();

     ui->plainTextEdit->clear();
     ui->plainTextEdit->appendPlainText(componentName + QLatin1String(" {"));
     if (!m_sourcePreview.first().isEmpty())
        ui->plainTextEdit->appendPlainText(m_sourcePreview.first());

     for (int i = 0; i < ui->listWidget->count(); ++i) {
         QListWidgetItem *item = ui->listWidget->item(i);

         if (item->checkState() == Qt::Checked)
             ui->plainTextEdit->appendPlainText(m_sourcePreview.at(i + 1));
     }

     ui->plainTextEdit->appendPlainText(QLatin1String("}"));
}

void ComponentNameDialog::validate()
{
    const QString message = isValid();
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(message.isEmpty());
    ui->messageLabel->setText(message);
}

QString ComponentNameDialog::isValid() const
{
    if (!ui->componentNameEdit->isValid())
        return ui->componentNameEdit->errorMessage();

    QString compName = ui->componentNameEdit->text();
    if (compName.isEmpty() || !compName[0].isUpper())
        return tr("Invalid component name");

    if (!ui->pathEdit->isValid())
        return tr("Invalid path");

    return QString();
}
