/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Dmitry Savchenko.
** Copyright (c) 2010 Vasiliy Sorokin.
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "keyworddialog.h"
#include "ui_keyworddialog.h"
#include "constants.h"

#include <QColorDialog>

namespace Todo {
namespace Internal {

KeywordDialog::KeywordDialog(const Keyword &keyword, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AddKeywordDialog)
{
    ui->setupUi(this);
    setupListWidget(keyword.iconResource);
    setupColorWidgets(keyword.color);
    ui->keywordNameEdit->setText(keyword.name);
}

KeywordDialog::~KeywordDialog()
{
    delete ui;
}

Keyword KeywordDialog::keyword()
{
    Keyword result;
    result.name = ui->keywordNameEdit->text();
    result.iconResource = ui->listWidget->currentItem()->data(Qt::UserRole).toString();
    result.color = ui->colorEdit->text();

    return result;
}

void KeywordDialog::colorSelected(const QColor &color)
{
    ui->colorEdit->setText(color.name());
}

void KeywordDialog::setupListWidget(const QString &selectedIcon)
{
    ui->listWidget->setViewMode(QListWidget::IconMode);

    QListWidgetItem *item = new QListWidgetItem(QIcon(Constants::ICON_INFO), "information");
    item->setData(Qt::UserRole, Constants::ICON_INFO);
    ui->listWidget->addItem(item);

    item = new QListWidgetItem(QIcon(Constants::ICON_WARNING), "warning");
    item->setData(Qt::UserRole, Constants::ICON_WARNING);
    ui->listWidget->addItem(item);

    item = new QListWidgetItem(QIcon(Constants::ICON_ERROR), "error");
    item->setData(Qt::UserRole, Constants::ICON_ERROR);
    ui->listWidget->addItem(item);

    for (int i = 0; i < ui->listWidget->count(); ++i) {
        item = ui->listWidget->item(i);
        if (item->data(Qt::UserRole).toString() == selectedIcon) {
            ui->listWidget->setCurrentItem(item);
            break;
        }
    }
}

void KeywordDialog::setupColorWidgets(const QColor &color)
{
    ui->colorButton->setColor(color);
    ui->colorEdit->setText(color.name());
    connect(ui->colorButton, SIGNAL(colorChanged(QColor)), SLOT(colorSelected(QColor)));
}

} // namespace Internal
} // namespace Todo
