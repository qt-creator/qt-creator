/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nicolas Arnaud-Cormos.
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
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
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "savedialog.h"
#include "ui_savedialog.h"

#include <QLineEdit>
#include <QCheckBox>
#include <QRegExpValidator>

using namespace Macros::Internal;

SaveDialog::SaveDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SaveDialog)
{
    ui->setupUi(this);
    ui->name->setValidator(new QRegExpValidator(QRegExp("\\w*"), this));
}

SaveDialog::~SaveDialog()
{
    delete ui;
}

QString SaveDialog::name() const
{
    return ui->name->text();
}

QString SaveDialog::description() const
{
    return ui->description->text();
}

bool SaveDialog::hideSaveDialog() const
{
    return ui->hideSaveDialog->isChecked();
}

bool SaveDialog::createShortcut() const
{
    return ui->createShortcut->isChecked();
}
