// Copyright (C) 2016 Brian McGillion
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "revertdialog.h"

namespace Mercurial {
namespace Internal  {

RevertDialog::RevertDialog(QWidget *parent) :
    QDialog(parent),
    m_ui(new Ui::RevertDialog)
{
    m_ui->setupUi(this);
}

RevertDialog::~RevertDialog()
{
    delete m_ui;
}

QString RevertDialog::revision() const
{
    return m_ui->revisionLineEdit->text();
}

} // namespace Internal
} // namespace Mercurial
