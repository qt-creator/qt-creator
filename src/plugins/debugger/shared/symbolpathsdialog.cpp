/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "symbolpathsdialog.h"
#include "ui_symbolpathsdialog.h"
#include "QMessageBox"

using namespace Debugger;
using namespace Internal;

SymbolPathsDialog::SymbolPathsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SymbolPathsDialog)
{
    ui->setupUi(this);
    ui->pixmapLabel->setPixmap(QMessageBox::standardIcon(QMessageBox::Question));
}

SymbolPathsDialog::~SymbolPathsDialog()
{
    delete ui;
}

bool SymbolPathsDialog::useSymbolCache() const
{
    return ui->useLocalSymbolCache->isChecked();
}

bool SymbolPathsDialog::useSymbolServer() const
{
    return ui->useSymbolServer->isChecked();
}

QString SymbolPathsDialog::path() const
{
    return ui->pathChooser->path();
}

bool SymbolPathsDialog::doNotAskAgain() const
{
    return ui->doNotAskAgain->isChecked();
}

void SymbolPathsDialog::setUseSymbolCache(bool useSymbolCache)
{
    ui->useLocalSymbolCache->setChecked(useSymbolCache);
}

void SymbolPathsDialog::setUseSymbolServer(bool useSymbolServer)
{
    ui->useSymbolServer->setChecked(useSymbolServer);
}

void SymbolPathsDialog::setPath(const QString &path)
{
    ui->pathChooser->setPath(path);
}

void SymbolPathsDialog::setDoNotAskAgain(bool doNotAskAgain) const
{
    ui->doNotAskAgain->setChecked(doNotAskAgain);
}

bool SymbolPathsDialog::useCommonSymbolPaths(bool &useSymbolCache, bool &useSymbolServer,
                                             QString &path, bool &doNotAskAgain)
{
    SymbolPathsDialog dialog;
    dialog.setUseSymbolCache(useSymbolCache);
    dialog.setUseSymbolServer(useSymbolServer);
    dialog.setPath(path);
    dialog.setDoNotAskAgain(doNotAskAgain);
    int ret = dialog.exec();
    useSymbolCache = dialog.useSymbolCache();
    useSymbolServer = dialog.useSymbolServer();
    path = dialog.path();
    doNotAskAgain = dialog.doNotAskAgain();
    return ret == QDialog::Accepted;
}
