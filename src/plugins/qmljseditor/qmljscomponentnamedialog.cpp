/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "qmljscomponentnamedialog.h"
#include "ui_qmljscomponentnamedialog.h"

#include <QtCore/QFileInfo>
#include <QtGui/QFileDialog>

using namespace QmlJSEditor::Internal;

ComponentNameDialog::ComponentNameDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ComponentNameDialog)
{
    ui->setupUi(this);

    connect(ui->choosePathButton, SIGNAL(clicked()),
            this, SLOT(choosePath()));
    connect(ui->pathEdit, SIGNAL(textChanged(QString)),
            this, SLOT(validate()));
    connect(ui->componentNameEdit, SIGNAL(textChanged(QString)),
            this, SLOT(validate()));
}

ComponentNameDialog::~ComponentNameDialog()
{
    delete ui;
}

void ComponentNameDialog::go(QString *proposedName,
                             QString *proposedPath,
                             QWidget *parent)
{
    Q_ASSERT(proposedName);
    Q_ASSERT(proposedPath);

    ComponentNameDialog d(parent);
    d.ui->componentNameEdit->setText(*proposedName);
    d.ui->pathEdit->setText(*proposedPath);

    if (QDialog::Accepted == d.exec()) {
        *proposedName = d.ui->componentNameEdit->text();
        *proposedPath = d.ui->pathEdit->text();
    }
}

void ComponentNameDialog::choosePath()
{
    QString dir = QFileDialog::getExistingDirectory(this, tr("Choose a path"),
                                                    ui->pathEdit->text());
    if (!dir.isEmpty())
        ui->pathEdit->setText(dir);
}

void ComponentNameDialog::validate()
{
    const QString msg = isValid();
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(msg.isEmpty());
    ui->messageLabel->setText(msg);
}

QString ComponentNameDialog::isValid() const
{
    QString compName = ui->componentNameEdit->text();
    if (compName.isEmpty() || !compName[0].isUpper())
        return tr("Invalid component name");

    QString path = ui->pathEdit->text();
    if (path.isEmpty() || !QFileInfo(path).isDir())
        return tr("Invalid path");

    return QString();
}
