#include "settingspage.h"
#include <coreplugin/icore.h>
#include <QList>
#include <QMessageBox>

SettingsPage::SettingsPage(KeywordsList keywords, int projectOptions, int paneOptions, QObject *parent) : IOptionsPage(parent)
{
    this->keywords = keywords;
    this->projectOptions = projectOptions;
    this->paneOptions = paneOptions;
}

SettingsPage::~SettingsPage()
{
    delete dialog;
}

QString SettingsPage::id() const
{
    return "TodoSettings";
}

QString SettingsPage::trName() const
{
    return tr("TODO Plugin");
}

QString SettingsPage::category() const
{
    return "TODO";
}

QString SettingsPage::trCategory() const
{
    return tr("TODO");
}

QString SettingsPage::displayName() const
{
    return trName();
}

QString SettingsPage::displayCategory() const
{
    return trCategory();
}

QIcon SettingsPage::categoryIcon() const
{
    return QIcon(":/todo");
}


QWidget *SettingsPage::createPage(QWidget *parent)
{
    settingsStatus = false;

    dialog = new SettingsDialog(parent);
    dialog->setKeywordsList(keywords);
    switch (projectOptions)
    {
    case 0:
        dialog->setProjectRadioButtonEnabled(false);
        dialog->setCurrentFileRadioButtonEnabled(true);
        break;
    case 1:
    default:
        dialog->setProjectRadioButtonEnabled(true);
        dialog->setCurrentFileRadioButtonEnabled(false);
        break;
    }

    switch (paneOptions)
    {
    case 0:
        dialog->setBuildIssuesRadioButtonEnabled(false);
        dialog->setTodoOutputRadioButtonEnabled(true);
        break;
    case 1:
    default:
        dialog->setBuildIssuesRadioButtonEnabled(true);
        dialog->setTodoOutputRadioButtonEnabled(false);
        break;
    }
    connect(dialog, SIGNAL(settingsChanged()), this, SLOT(settingsChanged()));
    return dialog;
}

void SettingsPage::apply()
{
    if (settingsStatus)
    {
        QSettings *settings = Core::ICore::instance()->settings();
        settings->beginGroup("TODOPlugin");
        projectOptions = dialog->currentFileRadioButtonEnabled() ? 0 : 1;
        paneOptions = dialog->todoOutputRadioButtonEnabled() ? 0 : 1;
        keywords = dialog->keywordsList();
        settings->setValue("project_options", projectOptions);
        settings->setValue("pane_options", paneOptions);
        settings->setValue("keywords", qVariantFromValue(keywords));

        settings->endGroup();
        settings->sync();

        QMessageBox::information(dialog, tr("Information"), tr("The TODO plugin settings change will take effect after a restart of Qt Creator."));
        settingsStatus = false;
    }
}

void SettingsPage::finish()
{
    //apply();
}

void SettingsPage::settingsChanged()
{
    settingsStatus = true;
}
