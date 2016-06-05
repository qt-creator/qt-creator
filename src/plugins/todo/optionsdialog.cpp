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
    ui->keywordsList->setIconSize(QSize(16, 16));
    setKeywordsButtonsEnabled();
    connect(ui->addKeywordButton, &QAbstractButton::clicked,
            this, &OptionsDialog::addKeywordButtonClicked);
    connect(ui->removeKeywordButton, &QAbstractButton::clicked,
            this, &OptionsDialog::removeKeywordButtonClicked);
    connect(ui->editKeywordButton, &QAbstractButton::clicked,
            this, &OptionsDialog::editKeywordButtonClicked);
    connect(ui->resetKeywordsButton, &QAbstractButton::clicked,
            this, &OptionsDialog::resetKeywordsButtonClicked);
    connect(ui->keywordsList, &QListWidget::itemDoubleClicked,
            this, &OptionsDialog::keywordDoubleClicked);
    connect(ui->keywordsList, &QListWidget::itemSelectionChanged,
            this, &OptionsDialog::setKeywordsButtonsEnabled);
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
    QListWidgetItem *item = new QListWidgetItem(
                icon(keyword.iconType), keyword.name);
    item->setData(Qt::UserRole, static_cast<int>(keyword.iconType));
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
    keyword.iconType = static_cast<IconType>(item->data(Qt::UserRole).toInt());
    keyword.color = item->backgroundColor();

    QSet<QString> keywordNamesButThis = keywordNames();
    keywordNamesButThis.remove(keyword.name);

    KeywordDialog *keywordDialog = new KeywordDialog(keyword, keywordNamesButThis, this);
    if (keywordDialog->exec() == QDialog::Accepted) {
        keyword = keywordDialog->keyword();
        item->setIcon(icon(keyword.iconType));
        item->setText(keyword.name);
        item->setData(Qt::UserRole, static_cast<int>(keyword.iconType));
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
    ui->scanInSubprojectRadioButton->setChecked(settings.scanningScope == ScanningScopeSubProject);

    ui->keywordsList->clear();
    foreach (const Keyword &keyword, settings.keywords)
        addToKeywordsList(keyword);
}

Settings OptionsDialog::settingsFromUi()
{
    Settings settings;

    if (ui->scanInCurrentFileRadioButton->isChecked())
        settings.scanningScope = ScanningScopeCurrentFile;
    else if (ui->scanInSubprojectRadioButton->isChecked())
        settings.scanningScope = ScanningScopeSubProject;
    else
        settings.scanningScope = ScanningScopeProject;

    settings.keywords.clear();
    for (int i = 0; i < ui->keywordsList->count(); ++i) {
        QListWidgetItem *item = ui->keywordsList->item(i);

        Keyword keyword;
        keyword.name = item->text();
        keyword.iconType = static_cast<IconType>(item->data(Qt::UserRole).toInt());
        keyword.color = item->backgroundColor();

        settings.keywords << keyword;
    }

    return settings;
}

} // namespace Internal
} // namespace Todo
