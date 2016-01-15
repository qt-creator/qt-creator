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

#include "symbolpathsdialog.h"
#include "ui_symbolpathsdialog.h"

#include <QMessageBox>

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

bool SymbolPathsDialog::useCommonSymbolPaths(bool &useSymbolCache, bool &useSymbolServer,
                                             QString &path)
{
    SymbolPathsDialog dialog;
    dialog.setUseSymbolCache(useSymbolCache);
    dialog.setUseSymbolServer(useSymbolServer);
    dialog.setPath(path);
    int ret = dialog.exec();
    useSymbolCache = dialog.useSymbolCache();
    useSymbolServer = dialog.useSymbolServer();
    path = dialog.path();
    return ret == QDialog::Accepted;
}
