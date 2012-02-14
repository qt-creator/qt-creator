#ifndef SETTINGSPAGE_H
#define SETTINGSPAGE_H

#include <QIcon>
#include <coreplugin/dialogs/ioptionspage.h>
#include "settingsdialog.h"


class SettingsPage : public Core::IOptionsPage
{
    Q_OBJECT
public:

    SettingsPage(KeywordsList keywords = KeywordsList(), int projectOptions = 0, int paneOptions = 0, QObject *parent = 0);
    ~SettingsPage();

    QString id() const;
    QString trName() const;
    QString category() const;
    QString trCategory() const;
    QString displayName() const;
    QIcon categoryIcon() const;
    QString displayCategory() const;
    QWidget *createPage(QWidget *parent);
    void apply();
    void finish();

public slots:
    void settingsChanged();

private:
    SettingsDialog *dialog;
    bool settingsStatus;
    int projectOptions;
    int paneOptions;
    KeywordsList keywords;
};

#endif // SETTINGSPAGE_H
