/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
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
**
**************************************************************************/

#include "mimetypemagicdialog.h"
#include "mimedatabase.h"

#include <QLatin1String>
#include <QMessageBox>

using namespace Core;
using namespace Internal;

MimeTypeMagicDialog::MimeTypeMagicDialog(QWidget *parent) :
    QDialog(parent)
{
    ui.setupUi(this);
    setWindowTitle(tr("Add Magic Header"));
    connect(ui.useRecommendedGroupBox, SIGNAL(clicked(bool)),
            this, SLOT(applyRecommended(bool)));
    connect(ui.buttonBox, SIGNAL(accepted()), this, SLOT(validateAccept()));
}

void MimeTypeMagicDialog::changeEvent(QEvent *e)
{
    QDialog::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui.retranslateUi(this);
        break;
    default:
        break;
    }
}

void MimeTypeMagicDialog::applyRecommended(bool checked)
{
    if (checked) {
        ui.startRangeSpinBox->setValue(0);
        ui.endRangeSpinBox->setValue(0);
        ui.prioritySpinBox->setValue(50);
    }
}

void MimeTypeMagicDialog::validateAccept()
{
    if (ui.valueLineEdit->text().isEmpty()
            || (ui.byteRadioButton->isChecked()
                && !Core::MagicByteRule::validateByteSequence(ui.valueLineEdit->text()))) {
        QMessageBox::critical(0, tr("Error"), tr("Not a valid byte pattern."));
        return;
    }
    accept();
}

void MimeTypeMagicDialog::setMagicData(const MagicData &data)
{
    ui.valueLineEdit->setText(data.m_value);
    if (data.m_type == Core::MagicStringRule::kMatchType)
        ui.stringRadioButton->setChecked(true);
    else
        ui.byteRadioButton->setChecked(true);
    ui.startRangeSpinBox->setValue(data.m_start);
    ui.endRangeSpinBox->setValue(data.m_end);
    ui.prioritySpinBox->setValue(data.m_priority);
}

MagicData MimeTypeMagicDialog::magicData() const
{
    MagicData data;
    data.m_value = ui.valueLineEdit->text();
    if (ui.stringRadioButton->isChecked())
        data.m_type = Core::MagicStringRule::kMatchType;
    else
        data.m_type = Core::MagicByteRule::kMatchType;
    data.m_start = ui.startRangeSpinBox->value();
    data.m_end = ui.endRangeSpinBox->value();
    data.m_priority = ui.prioritySpinBox->value();
    return data;
}
