/**************************************************************************
**
** Copyright (c) 2013 Dmitry Savchenko
** Copyright (c) 2013 Vasiliy Sorokin
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

#include "optionsdialog.h"
#include "ui_optionsdialog.h"
#include "keyworddialog.h"
#include "keyword.h"
#include "settings.h"
#include "constants.h"

namespace Todo {
namespace Internal {

OptionsDialog::OptionsDialog(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::OptionsDialog)
{
    ui->setupUi(this);
    setButtonsEnabled();
    connect(ui->addButton, SIGNAL(clicked()), SLOT(addButtonClicked()));
    connect(ui->removeButton, SIGNAL(clicked()), SLOT(removeButtonClicked()));
    connect(ui->editButton, SIGNAL(clicked()), SLOT(editButtonClicked()));
    connect(ui->resetButton, SIGNAL(clicked()), SLOT(resetButtonClicked()));
    connect(ui->keywordsList, SIGNAL(itemDoubleClicked(QListWidgetItem*)), SLOT(itemDoubleClicked(QListWidgetItem*)));
}

OptionsDialog::~OptionsDialog()
{
    delete ui;
}

void OptionsDialog::itemDoubleClicked(QListWidgetItem *item)
{
    editItem(item);
}

void OptionsDialog::setSettings(const Settings &settings)
{
    uiFromSettings(settings);
}

void OptionsDialog::addToKeywordsList(const Keyword &keyword)
{
    QListWidgetItem *item = new QListWidgetItem(QIcon(keyword.iconResource), keyword.name);
    item->setData(Qt::UserRole, keyword.iconResource);
    item->setBackgroundColor(keyword.color);
    ui->keywordsList->addItem(item);
}

QSet<QString> OptionsDialog::keywordNames()
{
    KeywordList keywords = settingsFromUi().keywords;

    QSet<QString> result;
    foreach (const Keyword &keyword, keywords)
        result << keyword.name;

    return result;
}

Settings OptionsDialog::settings()
{
    return settingsFromUi();
}

void OptionsDialog::addButtonClicked()
{
    Keyword keyword;
    KeywordDialog *keywordDialog = new KeywordDialog(keyword, keywordNames(), this);
    if (keywordDialog->exec() == QDialog::Accepted) {
        keyword = keywordDialog->keyword();
        addToKeywordsList(keyword);
    }
}

void OptionsDialog::editButtonClicked()
{
    QListWidgetItem *item = ui->keywordsList->currentItem();
    editItem(item);
}

void OptionsDialog::editItem(QListWidgetItem *item)
{
    Keyword keyword;
    keyword.name = item->text();
    keyword.iconResource = item->data(Qt::UserRole).toString();
    keyword.color = item->backgroundColor();

    QSet<QString> keywordNamesButThis = keywordNames();
    keywordNamesButThis.remove(keyword.name);

    KeywordDialog *keywordDialog = new KeywordDialog(keyword, keywordNamesButThis, this);
    if (keywordDialog->exec() == QDialog::Accepted) {
        keyword = keywordDialog->keyword();
        item->setIcon(QIcon(keyword.iconResource));
        item->setText(keyword.name);
        item->setData(Qt::UserRole, keyword.iconResource);
        item->setBackgroundColor(keyword.color);
    }
}

void OptionsDialog::removeButtonClicked()
{
    ui->keywordsList->takeItem(ui->keywordsList->currentRow());
}

void OptionsDialog::resetButtonClicked()
{
    Settings newSettings;
    newSettings.setDefault();
    uiFromSettings(newSettings);
}

void OptionsDialog::setButtonsEnabled()
{
    bool isSomethingSelected = ui->keywordsList->selectedItems().count() != 0;
    ui->removeButton->setEnabled(isSomethingSelected);
    ui->editButton->setEnabled(isSomethingSelected);
}

 void OptionsDialog::uiFromSettings(const Settings &settings)
 {
     ui->scanInCurrentFileRadioButton->setChecked(settings.scanningScope == ScanningScopeCurrentFile);
     ui->scanInProjectRadioButton->setChecked(settings.scanningScope == ScanningScopeProject);

     ui->keywordsList->clear();
     foreach (const Keyword &keyword, settings.keywords)
         addToKeywordsList(keyword);
 }

 Settings OptionsDialog::settingsFromUi()
 {
     Settings settings;

     if (ui->scanInCurrentFileRadioButton->isChecked())
         settings.scanningScope = ScanningScopeCurrentFile;
     else
         settings.scanningScope = ScanningScopeProject;

     settings.keywords.clear();
     for (int i = 0; i < ui->keywordsList->count(); ++i) {
         QListWidgetItem *item = ui->keywordsList->item(i);

         Keyword keyword;
         keyword.name = item->text();
         keyword.iconResource = item->data(Qt::UserRole).toString();
         keyword.color = item->backgroundColor();

         settings.keywords << keyword;
     }

     return settings;
 }

} // namespace Internal
} // namespace Todo
