/**************************************************************************
**
** Copyright (C) 2015 Dmitry Savchenko
** Copyright (C) 2015 Vasiliy Sorokin
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
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
    setKeywordsButtonsEnabled();
    connect(ui->addKeywordButton, SIGNAL(clicked()), SLOT(addKeywordButtonClicked()));
    connect(ui->removeKeywordButton, SIGNAL(clicked()), SLOT(removeKeywordButtonClicked()));
    connect(ui->editKeywordButton, SIGNAL(clicked()), SLOT(editKeywordButtonClicked()));
    connect(ui->resetKeywordsButton, SIGNAL(clicked()), SLOT(resetKeywordsButtonClicked()));
    connect(ui->keywordsList, SIGNAL(itemDoubleClicked(QListWidgetItem*)), SLOT(keywordDoubleClicked(QListWidgetItem*)));
}

OptionsDialog::~OptionsDialog()
{
    delete ui;
}

void OptionsDialog::keywordDoubleClicked(QListWidgetItem *item)
{
    editKeyword(item);
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

void OptionsDialog::addKeywordButtonClicked()
{
    Keyword keyword;
    KeywordDialog *keywordDialog = new KeywordDialog(keyword, keywordNames(), this);
    if (keywordDialog->exec() == QDialog::Accepted) {
        keyword = keywordDialog->keyword();
        addToKeywordsList(keyword);
    }
}

void OptionsDialog::editKeywordButtonClicked()
{
    QListWidgetItem *item = ui->keywordsList->currentItem();
    editKeyword(item);
}

void OptionsDialog::editKeyword(QListWidgetItem *item)
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

void OptionsDialog::removeKeywordButtonClicked()
{
    delete ui->keywordsList->takeItem(ui->keywordsList->currentRow());
}

void OptionsDialog::resetKeywordsButtonClicked()
{
    Settings newSettings;
    newSettings.setDefault();
    uiFromSettings(newSettings);
}

void OptionsDialog::setKeywordsButtonsEnabled()
{
    bool isSomethingSelected = ui->keywordsList->selectedItems().count() != 0;
    ui->removeKeywordButton->setEnabled(isSomethingSelected);
    ui->editKeywordButton->setEnabled(isSomethingSelected);
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
