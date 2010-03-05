/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#include "trkoptionswidget.h"
#include "trkoptions.h"
#include "debuggerconstants.h"
#include "ui_trkoptionswidget.h"

#include <QtCore/QTextStream>

namespace Debugger {
namespace Internal {

TrkOptionsWidget::TrkOptionsWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::TrkOptionsWidget)
{
    ui->setupUi(this);
    ui->gdbChooser->setExpectedKind(Utils::PathChooser::Command);
    ui->blueToothComboBox->addItems(TrkOptions::blueToothDevices());
    ui->serialComboBox->addItems(TrkOptions::serialPorts());
    connect(ui->commComboBox, SIGNAL(currentIndexChanged(int)), ui->commStackedWidget, SLOT(setCurrentIndex(int)));
    // No bluetooth on Windows yet...
#ifdef Q_OS_WIN
    ui->commGroupBox->setVisible(false);
#endif
}

TrkOptionsWidget::~TrkOptionsWidget()
{
    delete ui;
}

void TrkOptionsWidget::changeEvent(QEvent *e)
{
    QWidget::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

void TrkOptionsWidget::setTrkOptions(const TrkOptions &o)
{
    ui->commComboBox->setCurrentIndex(o.mode);
    ui->commStackedWidget->setCurrentIndex(o.mode);
    const int serialPortIndex = qMax(0, ui->serialComboBox->findText(o.serialPort));
    ui->serialComboBox->setCurrentIndex(serialPortIndex);
    ui->gdbChooser->setPath(o.gdb);
    const int blueToothIndex = qMax(0, ui->blueToothComboBox->findText(o.blueToothDevice));
    ui->blueToothComboBox->setCurrentIndex(blueToothIndex);
}

TrkOptions TrkOptionsWidget::trkOptions() const
{
    TrkOptions rc;
    rc.mode = ui->commComboBox->currentIndex();
    rc.gdb = ui->gdbChooser->path();
    rc.blueToothDevice = ui->blueToothComboBox->currentText();
    rc.serialPort = ui->serialComboBox->currentText();
    return rc;
}

QString TrkOptionsWidget::searchKeywords() const
{
    QString rc;
    QTextStream(&rc)  << ui->gdbLabel->text()  << ' ' << ui->serialLabel->text()
            << ' ' << ui->blueToothLabel->text();
    rc.remove(QLatin1Char('&'));
    return rc;
}

} // namespace Internal
} // namespace Designer
