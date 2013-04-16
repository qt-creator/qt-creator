/**************************************************************************
**
** Copyright (c) 2013 Bojan Petrovic
** Copyright (c) 2013 Radovan Zivkvoic
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
#include "schemaoptionswidget.h"
#include "ui_schemaoptionswidget.h"

#include <QFileDialog>

namespace VcProjectManager {
namespace Internal {

SchemaOptionsWidget::SchemaOptionsWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SchemaOptionsWidget)
{
    ui->setupUi(this);

    connect(ui->m_schema2003BrowseButton, SIGNAL(clicked()), this, SLOT(onBrowseSchema2003ButtonClick()));
    connect(ui->m_schema2005BrowseButton, SIGNAL(clicked()), this, SLOT(onBrowseSchema2005ButtonClick()));
    connect(ui->m_schema2008BrowseButton, SIGNAL(clicked()), this, SLOT(onBrowseSchema2008ButtonClick()));
}

SchemaOptionsWidget::~SchemaOptionsWidget()
{
    delete ui;
}

QString SchemaOptionsWidget::schemaPath(Constants::SchemaVersion schemaVersion)
{
    switch (schemaVersion) {
    case Constants::SV_2003:
        return ui->m_schema2003LineEdit->text();
    case Constants::SV_2005:
        return ui->m_schema2005LineEdit->text();
    case Constants::SV_2008:
        return ui->m_schema2008LineEdit->text();
    }

    return QString();
}

void SchemaOptionsWidget::setSchemaPath(const QString &schemaPath, Constants::SchemaVersion schemaVersion)
{
    switch (schemaVersion) {
    case Constants::SV_2003:
        ui->m_schema2003LineEdit->setText(schemaPath);
        break;
    case Constants::SV_2005:
        ui->m_schema2005LineEdit->setText(schemaPath);
        break;
    case Constants::SV_2008:
        ui->m_schema2008LineEdit->setText(schemaPath);
        break;
    }
}

void SchemaOptionsWidget::onBrowseSchema2003ButtonClick()
{
    QString filePath = QFileDialog::getOpenFileName(this, tr("Choose xml schema for 2003"), QString(), QLatin1String("*.xsd"));
    ui->m_schema2003LineEdit->setText(filePath);
}

void SchemaOptionsWidget::onBrowseSchema2005ButtonClick()
{
    QString filePath = QFileDialog::getOpenFileName(this, tr("Choose xml schema for 2005"), QString(), QLatin1String("*.xsd"));
    ui->m_schema2005LineEdit->setText(filePath);
}

void SchemaOptionsWidget::onBrowseSchema2008ButtonClick()
{
    QString filePath = QFileDialog::getOpenFileName(this, tr("Choose xml schema for 2008"), QString(), QLatin1String("*.xsd"));
    ui->m_schema2008LineEdit->setText(filePath);
}

} // namespace Internal
} // namespace VcProjectManager
