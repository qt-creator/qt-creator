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

#include <QtGui/qvalidator.h>

namespace QmlDesigner {

SetFrameValueDialog::SetFrameValueDialog(qreal frame, const QVariant &value,
                                         const QString &propertyName, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::SetFrameValueDialog)
{
    ui->setupUi(this);
    setWindowTitle(tr("Edit Keyframe"));
    setFixedSize(size());

    ui->lineEditFrame->setValidator(new QIntValidator(0, 99999, this));
    auto dv = new QDoubleValidator(this);
    dv->setDecimals(2);
    ui->lineEditValue->setValidator(dv);

    QLocale l;
    ui->lineEditFrame->setText(l.toString(qRound(frame)));
    ui->lineEditValue->setText(l.toString(value.toDouble(), 'f', 2));
    ui->labelValue->setText(propertyName);
}

SetFrameValueDialog::~SetFrameValueDialog()
{
    delete ui;
}

qreal SetFrameValueDialog::frame() const
{
    QLocale l;
    return l.toDouble(ui->lineEditFrame->text());
}

QVariant SetFrameValueDialog::value() const
{
    QLocale l;
    return QVariant(l.toDouble(ui->lineEditValue->text()));
}

} // namespace QmlDesigner
