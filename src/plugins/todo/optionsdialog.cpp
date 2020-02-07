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

class OptionsDialog final : public Core::IOptionsPageWidget
{
    Q_DECLARE_TR_FUNCTIONS(Todo::Internal::TodoOptionsPage)

public:
    OptionsDialog(Settings *settings, const std::function<void ()> &onApply);

    void apply() final;

    void setSettings(const Settings &settings);

private:
    void addKeywordButtonClicked();
    void editKeywordButtonClicked();
    void removeKeywordButtonClicked();
    void resetKeywordsButtonClicked();
    void setKeywordsButtonsEnabled();
    Settings settingsFromUi();
    void addToKeywordsList(const Keyword &keyword);
    void editKeyword(QListWidgetItem *item);
    QSet<QString> keywordNames();

    Ui::OptionsDialog ui;
    Settings *m_settings = nullptr;
    std::function<void()> m_onApply;
};

OptionsDialog::OptionsDialog(Settings *settings, const std::function<void ()> &onApply)
    : m_settings(settings), m_onApply(onApply)
{
    ui.setupUi(this);
    ui.keywordsList->setIconSize(QSize(16, 16));
    setKeywordsButtonsEnabled();
    connect(ui.addKeywordButton, &QAbstractButton::clicked,
            this, &OptionsDialog::addKeywordButtonClicked);
    connect(ui.removeKeywordButton, &QAbstractButton::clicked,
            this, &OptionsDialog::removeKeywordButtonClicked);
    connect(ui.editKeywordButton, &QAbstractButton::clicked,
            this, &OptionsDialog::editKeywordButtonClicked);
    connect(ui.resetKeywordsButton, &QAbstractButton::clicked,
            this, &OptionsDialog::resetKeywordsButtonClicked);
    connect(ui.keywordsList, &QListWidget::itemDoubleClicked,
            this, &OptionsDialog::editKeyword);
    connect(ui.keywordsList, &QListWidget::itemSelectionChanged,
            this, &OptionsDialog::setKeywordsButtonsEnabled);

    setSettings(*m_settings);
}

void OptionsDialog::addToKeywordsList(const Keyword &keyword)
{
    auto item = new QListWidgetItem(icon(keyword.iconType), keyword.name);
    item->setData(Qt::UserRole, static_cast<int>(keyword.iconType));
    item->setForeground(keyword.color);
    ui.keywordsList->addItem(item);
}

QSet<QString> OptionsDialog::keywordNames()
{
    const KeywordList keywords = settingsFromUi().keywords;

    QSet<QString> result;
    for (const Keyword &keyword : keywords)
        result << keyword.name;

    return result;
}

void OptionsDialog::addKeywordButtonClicked()
{
    Keyword keyword;
    KeywordDialog keywordDialog(keyword, keywordNames(), this);
    if (keywordDialog.exec() == QDialog::Accepted) {
        keyword = keywordDialog.keyword();
        addToKeywordsList(keyword);
    }
}

void OptionsDialog::editKeywordButtonClicked()
{
    QListWidgetItem *item = ui.keywordsList->currentItem();
    editKeyword(item);
}

void OptionsDialog::editKeyword(QListWidgetItem *item)
{
    Keyword keyword;
    keyword.name = item->text();
    keyword.iconType = static_cast<IconType>(item->data(Qt::UserRole).toInt());
    keyword.color = item->foreground().color();

    QSet<QString> keywordNamesButThis = keywordNames();
    keywordNamesButThis.remove(keyword.name);

    KeywordDialog keywordDialog(keyword, keywordNamesButThis, this);
    if (keywordDialog.exec() == QDialog::Accepted) {
        keyword = keywordDialog.keyword();
        item->setIcon(icon(keyword.iconType));
        item->setText(keyword.name);
        item->setData(Qt::UserRole, static_cast<int>(keyword.iconType));
        item->setForeground(keyword.color);
    }
}

void OptionsDialog::removeKeywordButtonClicked()
{
    delete ui.keywordsList->takeItem(ui.keywordsList->currentRow());
}

void OptionsDialog::resetKeywordsButtonClicked()
{
    Settings newSettings;
    newSettings.setDefault();
    setSettings(newSettings);
}

void OptionsDialog::setKeywordsButtonsEnabled()
{
    const bool isSomethingSelected = !ui.keywordsList->selectedItems().isEmpty();
    ui.removeKeywordButton->setEnabled(isSomethingSelected);
    ui.editKeywordButton->setEnabled(isSomethingSelected);
}

void OptionsDialog::setSettings(const Settings &settings)
{
    ui.scanInCurrentFileRadioButton->setChecked(settings.scanningScope == ScanningScopeCurrentFile);
    ui.scanInProjectRadioButton->setChecked(settings.scanningScope == ScanningScopeProject);
    ui.scanInSubprojectRadioButton->setChecked(settings.scanningScope == ScanningScopeSubProject);

    ui.keywordsList->clear();
    for (const Keyword &keyword : qAsConst(settings.keywords))
        addToKeywordsList(keyword);
}

Settings OptionsDialog::settingsFromUi()
{
    Settings settings;

    if (ui.scanInCurrentFileRadioButton->isChecked())
        settings.scanningScope = ScanningScopeCurrentFile;
    else if (ui.scanInSubprojectRadioButton->isChecked())
        settings.scanningScope = ScanningScopeSubProject;
    else
        settings.scanningScope = ScanningScopeProject;

    settings.keywords.clear();
    for (int i = 0; i < ui.keywordsList->count(); ++i) {
        QListWidgetItem *item = ui.keywordsList->item(i);

        Keyword keyword;
        keyword.name = item->text();
        keyword.iconType = static_cast<IconType>(item->data(Qt::UserRole).toInt());
        keyword.color = item->foreground().color();

        settings.keywords << keyword;
    }

    return settings;
}

void OptionsDialog::apply()
{
    Settings newSettings = settingsFromUi();

    // "apply" itself is interpreted as "use these keywords, also for other themes".
    newSettings.keywordsEdited = true;

    if (newSettings == *m_settings)
        return;

    *m_settings = newSettings;
    m_onApply();
}

// TodoOptionsPage

TodoOptionsPage::TodoOptionsPage(Settings *settings, const std::function<void ()> &onApply)
{
    setId("TodoSettings");
    setDisplayName(OptionsDialog::tr("To-Do"));
    setCategory("To-Do");
    setDisplayCategory(OptionsDialog::tr("To-Do"));
    setCategoryIconPath(":/todoplugin/images/settingscategory_todo.png");
    setWidgetCreator([settings, onApply] { return new OptionsDialog(settings, onApply); });
}

} // namespace Internal
} // namespace Todo
