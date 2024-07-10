// Copyright (C) 2016 Dmitry Savchenko
// Copyright (C) 2016 Vasiliy Sorokin
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "settings.h"

#include "constants.h"
#include "keyword.h"
#include "keyworddialog.h"
#include "todoitemsprovider.h"
#include "todooutputpane.h"
#include "todotr.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/dialogs/ioptionspage.h>
#include <coreplugin/icore.h>

#include <utils/layoutbuilder.h>
#include <utils/qtcsettings.h>
#include <utils/theme/theme.h>

#include <QGroupBox>
#include <QListWidget>
#include <QPushButton>
#include <QRadioButton>

using namespace Core;
using namespace Utils;

namespace Todo::Internal {

Settings &todoSettings()
{
    static Settings theTodoSettings;
    return theTodoSettings;
}

void Settings::save() const
{
    if (!keywordsEdited)
        return;

    QtcSettings *settings = ICore::settings();
    settings->beginGroup(Constants::SETTINGS_GROUP);
    settings->setValue(Constants::SCANNING_SCOPE, scanningScope);

    settings->beginWriteArray(Constants::KEYWORDS_LIST);
    if (const int size = keywords.size()) {
        const Key nameKey = "name";
        const Key colorKey = "color";
        const Key iconTypeKey = "iconType";
        for (int i = 0; i < size; ++i) {
            settings->setArrayIndex(i);
            settings->setValue(nameKey, keywords.at(i).name);
            settings->setValue(colorKey, keywords.at(i).color);
            settings->setValue(iconTypeKey, static_cast<int>(keywords.at(i).iconType));
        }
    }
    settings->endArray();

    settings->endGroup();
    settings->sync();
}

void Settings::load()
{
    setDefault();

    QtcSettings *settings = ICore::settings();
    settings->beginGroup(Constants::SETTINGS_GROUP);

    scanningScope = static_cast<ScanningScope>(settings->value(Constants::SCANNING_SCOPE,
        ScanningScopeCurrentFile).toInt());
    if (scanningScope >= ScanningScopeMax)
        scanningScope = ScanningScopeCurrentFile;

    KeywordList newKeywords;
    const int keywordsSize = settings->beginReadArray(Constants::KEYWORDS_LIST);
    if (keywordsSize > 0) {
        const Key nameKey = "name";
        const Key colorKey = "color";
        const Key iconTypeKey = "iconType";
        for (int i = 0; i < keywordsSize; ++i) {
            settings->setArrayIndex(i);
            Keyword keyword;
            keyword.name = settings->value(nameKey).toString();
            keyword.color = settings->value(colorKey).value<QColor>();
            keyword.iconType = static_cast<IconType>(settings->value(iconTypeKey).toInt());
            newKeywords << keyword;
        }
        keywords = newKeywords;
        keywordsEdited = true; // Otherwise they wouldn't have been saved
    }
    settings->endArray();

    settings->endGroup();
}

void Settings::setDefault()
{
    scanningScope = ScanningScopeCurrentFile;

    keywords.clear();

    Keyword keyword;

    keyword.name = "TODO";
    keyword.iconType = IconType::Todo;
    keyword.color = creatorColor(Utils::Theme::OutputPanes_NormalMessageTextColor);
    keywords.append(keyword);

    keyword.name = R"(\todo)";
    keyword.iconType = IconType::Todo;
    keyword.color = creatorColor(Utils::Theme::OutputPanes_NormalMessageTextColor);
    keywords.append(keyword);

    keyword.name = "NOTE";
    keyword.iconType = IconType::Info;
    keyword.color = creatorColor(Utils::Theme::OutputPanes_NormalMessageTextColor);
    keywords.append(keyword);

    keyword.name = "FIXME";
    keyword.iconType = IconType::Error;
    keyword.color = creatorColor(Utils::Theme::OutputPanes_ErrorMessageTextColor);
    keywords.append(keyword);

    keyword.name = "BUG";
    keyword.iconType = IconType::Bug;
    keyword.color = creatorColor(Utils::Theme::OutputPanes_ErrorMessageTextColor);
    keywords.append(keyword);

    keyword.name = "WARNING";
    keyword.iconType = IconType::Warning;
    keyword.color = creatorColor(Utils::Theme::OutputPanes_WarningMessageTextColor);
    keywords.append(keyword);

    keywordsEdited = false;
}

static bool operator==(const Settings &s1, const Settings &s2)
{
    return s1.keywords == s2.keywords
        && s1.scanningScope == s2.scanningScope
        && s1.keywordsEdited == s2.keywordsEdited;
}

class OptionsDialog final : public Core::IOptionsPageWidget
{
public:
    OptionsDialog();

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

    QListWidget *m_keywordsList;
    QPushButton *m_editKeywordButton;
    QPushButton *m_removeKeywordButton;
    QPushButton *resetKeywordsButton;
    QRadioButton *m_scanInProjectRadioButton;
    QRadioButton *m_scanInCurrentFileRadioButton;
    QRadioButton *m_scanInSubprojectRadioButton;
};

OptionsDialog::OptionsDialog()
{
    m_keywordsList = new QListWidget;
    m_keywordsList->setDragDropMode(QAbstractItemView::DragDrop);
    m_keywordsList->setDefaultDropAction(Qt::MoveAction);
    m_keywordsList->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_keywordsList->setSortingEnabled(false);

    auto addKeywordButton = new QPushButton(Tr::tr("Add"));
    m_editKeywordButton = new QPushButton(Tr::tr("Edit"));
    m_removeKeywordButton = new QPushButton(Tr::tr("Remove"));
    resetKeywordsButton = new QPushButton(Tr::tr("Reset"));

    m_scanInProjectRadioButton = new QRadioButton(Tr::tr("Scan the whole active project"));
    m_scanInProjectRadioButton->setEnabled(true);

    m_scanInCurrentFileRadioButton = new QRadioButton(Tr::tr("Scan only the currently edited document"));
    m_scanInCurrentFileRadioButton->setChecked(true);

    m_scanInSubprojectRadioButton = new QRadioButton(Tr::tr("Scan the current subproject"));

    using namespace Layouting;

    Column {
        Group {
            title(Tr::tr("Keywords")),
            Row {
                m_keywordsList,
                Column {
                    addKeywordButton,
                    m_editKeywordButton,
                    m_removeKeywordButton,
                    resetKeywordsButton,
                    st
                }
            }
        },
        Group {
            title(Tr::tr("Scanning Scope")),
            Column {
                m_scanInProjectRadioButton,
                m_scanInCurrentFileRadioButton,
                m_scanInSubprojectRadioButton
            }
        }
    }.attachTo(this);

    m_keywordsList->setIconSize(QSize(16, 16));
    setKeywordsButtonsEnabled();
    connect(addKeywordButton, &QAbstractButton::clicked,
            this, &OptionsDialog::addKeywordButtonClicked);
    connect(m_removeKeywordButton, &QAbstractButton::clicked,
            this, &OptionsDialog::removeKeywordButtonClicked);
    connect(m_editKeywordButton, &QAbstractButton::clicked,
            this, &OptionsDialog::editKeywordButtonClicked);
    connect(resetKeywordsButton, &QAbstractButton::clicked,
            this, &OptionsDialog::resetKeywordsButtonClicked);
    connect(m_keywordsList, &QListWidget::itemDoubleClicked,
            this, &OptionsDialog::editKeyword);
    connect(m_keywordsList, &QListWidget::itemSelectionChanged,
            this, &OptionsDialog::setKeywordsButtonsEnabled);

    setSettings(todoSettings());
}

void OptionsDialog::addToKeywordsList(const Keyword &keyword)
{
    auto item = new QListWidgetItem(icon(keyword.iconType), keyword.name);
    item->setData(Qt::UserRole, static_cast<int>(keyword.iconType));
    item->setForeground(keyword.color);
    m_keywordsList->addItem(item);
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
    QListWidgetItem *item = m_keywordsList->currentItem();
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
    delete m_keywordsList->takeItem(m_keywordsList->currentRow());
}

void OptionsDialog::resetKeywordsButtonClicked()
{
    Settings newSettings;
    newSettings.setDefault();
    setSettings(newSettings);
}

void OptionsDialog::setKeywordsButtonsEnabled()
{
    const bool isSomethingSelected = !m_keywordsList->selectedItems().isEmpty();
    m_removeKeywordButton->setEnabled(isSomethingSelected);
    m_editKeywordButton->setEnabled(isSomethingSelected);
}

void OptionsDialog::setSettings(const Settings &settings)
{
    m_scanInCurrentFileRadioButton->setChecked(settings.scanningScope == ScanningScopeCurrentFile);
    m_scanInProjectRadioButton->setChecked(settings.scanningScope == ScanningScopeProject);
    m_scanInSubprojectRadioButton->setChecked(settings.scanningScope == ScanningScopeSubProject);

    m_keywordsList->clear();
    for (const Keyword &keyword : std::as_const(settings.keywords))
        addToKeywordsList(keyword);
}

Settings OptionsDialog::settingsFromUi()
{
    Settings settings;

    if (m_scanInCurrentFileRadioButton->isChecked())
        settings.scanningScope = ScanningScopeCurrentFile;
    else if (m_scanInSubprojectRadioButton->isChecked())
        settings.scanningScope = ScanningScopeSubProject;
    else
        settings.scanningScope = ScanningScopeProject;

    settings.keywords.clear();
    for (int i = 0; i < m_keywordsList->count(); ++i) {
        QListWidgetItem *item = m_keywordsList->item(i);

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

    if (newSettings == todoSettings())
        return;

    todoSettings() = newSettings;

    todoSettings().save();

    todoItemsProvider().settingsChanged();
    todoOutputPane().setScanningScope(todoSettings().scanningScope);
}

// TodoSettingsPage

class TodoSettingsPage final : public IOptionsPage
{
public:
    TodoSettingsPage()
    {
        setId(Constants::TODO_SETTINGS);
        setDisplayName(Tr::tr("To-Do"));
        setCategory("To-Do");
        setDisplayCategory(Tr::tr("To-Do"));
        setCategoryIconPath(":/todoplugin/images/settingscategory_todo.png");
        setWidgetCreator([] { return new OptionsDialog; });
    }
};

void setupTodoSettingsPage()
{
    static TodoSettingsPage theTodoSettingsPage;

    QObject::connect(ICore::instance(), &Core::ICore::saveSettingsRequested,
                     [] { todoSettings().save(); });
}

} // Todo::Internal

