/****************************************************************************
**
** Copyright (C) 2016 Dmitry Savchenko
** Copyright (C) 2016 Vasiliy Sorokin
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

#include "keyworddialog.h"
#include "keyword.h"
#include "ui_keyworddialog.h"
#include "constants.h"
#include "lineparser.h"

#include <QColorDialog>

namespace Todo {
namespace Internal {

KeywordDialog::KeywordDialog(const Keyword &keyword, const QSet<QString> &alreadyUsedKeywordNames,
                             QWidget *parent) :
    QDialog(parent),
    ui(new Ui::KeywordDialog),
    m_alreadyUsedKeywordNames(alreadyUsedKeywordNames)
{
    ui->setupUi(this);
    setupListWidget(keyword.iconType);
    setupColorWidgets(keyword.color);
    ui->keywordNameEdit->setText(keyword.name);
    ui->errorLabel->hide();

    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &KeywordDialog::acceptButtonClicked);
    connect(ui->keywordNameEdit, &QLineEdit::textChanged, ui->errorLabel, &QWidget::hide);
}

KeywordDialog::~KeywordDialog()
{
    delete ui;
}

Keyword KeywordDialog::keyword()
{
    Keyword result;
    result.name = keywordName();
    result.iconType = static_cast<IconType>(ui->listWidget->currentItem()->data(Qt::UserRole).toInt());
    result.color = ui->colorEdit->text();

    return result;
}

void KeywordDialog::colorSelected(const QColor &color)
{
    ui->colorEdit->setText(color.name());
}

void KeywordDialog::acceptButtonClicked()
{
    if (canAccept())
        accept();
}

void KeywordDialog::setupListWidget(IconType selectedIcon)
{
    ui->listWidget->setViewMode(QListWidget::IconMode);
    ui->listWidget->setDragEnabled(false);

    QListWidgetItem *item = new QListWidgetItem(icon(IconType::Info), "information");
    item->setData(Qt::UserRole, static_cast<int>(IconType::Info));
    ui->listWidget->addItem(item);

    item = new QListWidgetItem(icon(IconType::Warning), "warning");
    item->setData(Qt::UserRole, static_cast<int>(IconType::Warning));
    ui->listWidget->addItem(item);

    item = new QListWidgetItem(icon(IconType::Error), "error");
    item->setData(Qt::UserRole, static_cast<int>(IconType::Error));
    ui->listWidget->addItem(item);

    item = new QListWidgetItem(icon(IconType::Bug), "bug");
    item->setData(Qt::UserRole, static_cast<int>(IconType::Bug));
    ui->listWidget->addItem(item);

    item = new QListWidgetItem(icon(IconType::Todo), "todo");
    item->setData(Qt::UserRole, static_cast<int>(IconType::Todo));
    ui->listWidget->addItem(item);

    for (int i = 0; i < ui->listWidget->count(); ++i) {
        item = ui->listWidget->item(i);
        if (static_cast<IconType>(item->data(Qt::UserRole).toInt()) == selectedIcon) {
            ui->listWidget->setCurrentItem(item);
            break;
        }
    }
}

void KeywordDialog::setupColorWidgets(const QColor &color)
{
    ui->colorButton->setColor(color);
    ui->colorEdit->setText(color.name());
    connect(ui->colorButton, &Utils::QtColorButton::colorChanged, this, &KeywordDialog::colorSelected);
}

bool KeywordDialog::canAccept()
{
    if (!isKeywordNameCorrect()) {
        showError(tr("Keyword cannot be empty, contain spaces, colons, slashes or asterisks."));
        return false;
    }

    if (isKeywordNameAlreadyUsed()) {
        showError(tr("There is already a keyword with this name."));
        return false;
    }

    return true;
}

bool KeywordDialog::isKeywordNameCorrect()
{
    // Make sure keyword is not empty and contains no spaces or colons

    QString name = keywordName();

    if (name.isEmpty())
        return false;

    for (int i = 0; i < name.size(); ++i)
        if (LineParser::isKeywordSeparator(name.at(i)))
            return false;

    return true;
}

bool KeywordDialog::isKeywordNameAlreadyUsed()
{
    return m_alreadyUsedKeywordNames.contains(keywordName());
}

void KeywordDialog::showError(const QString &text)
{
    ui->errorLabel->setText(text);
    ui->errorLabel->show();
}

QString KeywordDialog::keywordName()
{
    return ui->keywordNameEdit->text().trimmed();
}

} // namespace Internal
} // namespace Todo
