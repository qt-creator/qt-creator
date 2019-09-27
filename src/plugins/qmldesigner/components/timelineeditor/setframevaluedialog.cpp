/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include "setframevaluedialog.h"
#include "ui_setframevaluedialog.h"

#include <QIntValidator>

namespace QmlDesigner {

SetFrameValueDialog::SetFrameValueDialog(qreal frame, const QVariant &value,
                                         const QString &propertyName, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::SetFrameValueDialog)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setWindowTitle(tr("Edit Keyframe"));
    setFixedSize(size());

    ui->lineEditFrame->setValidator(new QIntValidator(0, 99999, this));

    ui->lineEditFrame->setText(QString::number(frame));
    ui->lineEditValue->setText(value.toString());
    ui->labelValue->setText(propertyName);
}

SetFrameValueDialog::~SetFrameValueDialog()
{
    delete ui;
}

qreal SetFrameValueDialog::frame() const
{
    return ui->lineEditFrame->text().toDouble();
}

QVariant SetFrameValueDialog::value() const
{
    return QVariant(ui->lineEditValue->text());
}

} // namespace QmlDesigner
