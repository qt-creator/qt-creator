/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "externaltoolconfig.h"
#include "ui_externaltoolconfig.h"

#include <QtCore/QTextStream>

using namespace Core::Internal;

ExternalToolConfig::ExternalToolConfig(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ExternalToolConfig)
{
    ui->setupUi(this);
    connect(ui->toolTree, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)),
            this, SLOT(showInfoForItem(QTreeWidgetItem*)));
    showInfoForItem(0);
}

ExternalToolConfig::~ExternalToolConfig()
{
    delete ui;
}

QString ExternalToolConfig::searchKeywords() const
{
    QString keywords;
    QTextStream(&keywords)
            << ui->descriptionLabel->text()
            << ui->executableLabel->text()
            << ui->argumentsLabel->text()
            << ui->workingDirectoryLabel->text()
            << ui->outputLabel->text()
            << ui->errorOutputLabel->text()
            << ui->inputCheckbox->text();
    return keywords;
}

void ExternalToolConfig::setTools(const QMap<QString, QList<ExternalTool *> > &tools)
{
    // TODO make copy of tools

    QMapIterator<QString, QList<ExternalTool *> > categories(tools);
    while (categories.hasNext()) {
        categories.next();
        QString name = (categories.key().isEmpty() ? tr("External Tools Menu") : categories.key());
        QTreeWidgetItem *category = new QTreeWidgetItem(ui->toolTree, QStringList() << name);
        foreach (ExternalTool *tool, categories.value()) {
            QTreeWidgetItem *item = new QTreeWidgetItem(category, QStringList() << tool->displayName());
            item->setData(0, Qt::UserRole, qVariantFromValue(tool));
        }
    }
    ui->toolTree->expandAll();
}

void ExternalToolConfig::showInfoForItem(QTreeWidgetItem *item)
{
    ExternalTool *tool = 0;
    if (item)
        tool = item->data(0, Qt::UserRole).value<ExternalTool *>();
    if (!tool) {
        ui->description->setText(QString());
        ui->executable->setPath(QString());
        ui->arguments->setText(QString());
        ui->workingDirectory->setPath(QString());
        ui->inputText->setPlainText(QString());
        ui->infoWidget->setEnabled(false);
        return;
    }
    ui->infoWidget->setEnabled(true);
    ui->description->setText(tool->description());
    ui->executable->setPath(tool->executables().isEmpty() ? QString() : tool->executables().first());
    ui->arguments->setText(tool->arguments());
    ui->workingDirectory->setPath(tool->workingDirectory());
    ui->outputBehavior->setCurrentIndex((int)tool->outputHandling());
    ui->errorOutputBehavior->setCurrentIndex((int)tool->errorHandling());
    ui->inputCheckbox->setChecked(!tool->input().isEmpty());
    ui->inputText->setPlainText(tool->input());
    ui->description->setCursorPosition(0);
    ui->arguments->setCursorPosition(0);
}
