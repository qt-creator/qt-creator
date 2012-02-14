#include "settingsdialog.h"
#include "ui_settingsdialog.h"
#include "addkeyworddialog.h"

SettingsDialog::SettingsDialog(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SettingsDialog)
{
    ui->setupUi(this);
    connect(this->ui->addButton, SIGNAL(clicked()), this, SLOT(addButtonClicked()));
    connect(this->ui->removeButton, SIGNAL(clicked()), this, SLOT(removeButtonClicked()));
    connect(this->ui->resetButton, SIGNAL(clicked()), this, SLOT(resetButtonClicked()));

    connect(this->ui->buildIssuesRadioButton, SIGNAL(toggled(bool)), this, SIGNAL(settingsChanged()));
    connect(this->ui->todoOutputRadioButton, SIGNAL(toggled(bool)), this, SIGNAL(settingsChanged()));
    connect(this->ui->projectRadioButton, SIGNAL(toggled(bool)), this, SIGNAL(settingsChanged()));
    connect(this->ui->currentFileRadioButton, SIGNAL(toggled(bool)), this, SIGNAL(settingsChanged()));

}

SettingsDialog::~SettingsDialog()
{
    delete ui;
}

void SettingsDialog::setProjectRadioButtonEnabled(bool what)
{
    this->ui->projectRadioButton->setChecked(what);
}

void SettingsDialog::setCurrentFileRadioButtonEnabled(bool what)
{
    this->ui->currentFileRadioButton->setChecked(what);
}

void SettingsDialog::setBuildIssuesRadioButtonEnabled(bool what)
{
    this->ui->buildIssuesRadioButton->setChecked(what);
}

void SettingsDialog::setTodoOutputRadioButtonEnabled(bool what)
{
    this->ui->todoOutputRadioButton->setChecked(what);
}


void SettingsDialog::addToKeywordsList(Keyword keyword)
{
    QListWidgetItem *item = new QListWidgetItem(keyword.icon, keyword.name);
    item->setBackgroundColor(keyword.warningColor);
    this->ui->keywordsList->addItem(item);
}

void SettingsDialog::setKeywordsList(KeywordsList list)
{
    if (!list.count())
    {
        resetButtonClicked();
    }
    else
    {
        for (int i = 0; i < list.count(); ++i)
        {
            addToKeywordsList(list.at(i));
        }
    }
}

bool SettingsDialog::projectRadioButtonEnabled()
{
    return this->ui->projectRadioButton->isChecked();
}

bool SettingsDialog::currentFileRadioButtonEnabled()
{
    return this->ui->currentFileRadioButton->isChecked();
}

bool SettingsDialog::buildIssuesRadioButtonEnabled()
{
    return this->ui->buildIssuesRadioButton->isChecked();
}

bool SettingsDialog::todoOutputRadioButtonEnabled()
{
    return this->ui->todoOutputRadioButton->isChecked();
}

KeywordsList SettingsDialog::keywordsList()
{
    KeywordsList list;
    for (int i = 0; i < this->ui->keywordsList->count(); ++i)
    {
        Keyword keyword;
        keyword.name = this->ui->keywordsList->item(i)->text();
        keyword.icon = this->ui->keywordsList->item(i)->icon();
        keyword.warningColor = this->ui->keywordsList->item(i)->backgroundColor();
        list.append(keyword);
    }
    return list;
}

void SettingsDialog::clearKeywordsList()
{
    this->ui->keywordsList->clear();
}

void SettingsDialog::addButtonClicked()
{
    Keyword keyword;
    AddKeywordDialog *addKeywordDialog = new AddKeywordDialog(this);
    if (addKeywordDialog->exec() == QDialog::Accepted)
    {
        keyword.name = addKeywordDialog->keywordName();
        keyword.icon = addKeywordDialog->keywordIcon();
        keyword.warningColor = addKeywordDialog->keywordColor();
        addToKeywordsList(keyword);
        emit settingsChanged();
    }
}

void SettingsDialog::removeButtonClicked()
{
        this->ui->keywordsList->takeItem(this->ui->keywordsList->currentRow());
        emit settingsChanged();
}

void SettingsDialog::resetButtonClicked()
{
    clearKeywordsList();
    addToKeywordsList(Keyword("TODO", QIcon(":/warning"), QColor("#BFFFC8")));
    addToKeywordsList(Keyword("NOTE", QIcon(":/info"), QColor("#E2DFFF")));
    addToKeywordsList(Keyword("FIXME", QIcon(":/error"), QColor("#FFBFBF")));
    addToKeywordsList(Keyword("BUG", QIcon(":/error"), QColor("#FFDFDF")));
    addToKeywordsList(Keyword("HACK", QIcon(":/info"), QColor("#FFFFAA")));

    emit settingsChanged();
}
