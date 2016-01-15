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

    connect(ui->pathEdit, SIGNAL(rawPathChanged(QString)),
            this, SLOT(validate()));
    connect(ui->componentNameEdit, SIGNAL(textChanged(QString)),
            this, SLOT(validate()));
}

ComponentNameDialog::~ComponentNameDialog()
{
    delete ui;
}

bool ComponentNameDialog::go(QString *proposedName,
                             QString *proposedPath,
                             QWidget *parent)
{
    Q_ASSERT(proposedName);
    Q_ASSERT(proposedPath);

    ComponentNameDialog d(parent);
    d.ui->componentNameEdit->setNamespacesEnabled(false);
    d.ui->componentNameEdit->setLowerCaseFileName(false);
    d.ui->componentNameEdit->setForceFirstCapitalLetter(true);
    d.ui->componentNameEdit->setText(*proposedName);
    d.ui->pathEdit->setExpectedKind(Utils::PathChooser::ExistingDirectory);
    d.ui->pathEdit->setHistoryCompleter(QLatin1String("QmlJs.Component.History"));
    d.ui->pathEdit->setPath(*proposedPath);

    if (QDialog::Accepted == d.exec()) {
        *proposedName = d.ui->componentNameEdit->text();
        *proposedPath = d.ui->pathEdit->path();
        return true;
    }

    return false;
}

void ComponentNameDialog::choosePath()
{
    QString dir = QFileDialog::getExistingDirectory(this, tr("Choose a path"),
                                                    ui->pathEdit->path());
    if (!dir.isEmpty())
        ui->pathEdit->setPath(dir);
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
