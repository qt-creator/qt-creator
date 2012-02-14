#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QWidget>
#include "keyword.h"

namespace Ui {
    class SettingsDialog;
}

class SettingsDialog : public QWidget
{
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget *parent = 0);
    ~SettingsDialog();
    void setProjectRadioButtonEnabled(bool what);
    void setCurrentFileRadioButtonEnabled(bool what);
    void setBuildIssuesRadioButtonEnabled(bool what);
    void setTodoOutputRadioButtonEnabled(bool what);

    void addToKeywordsList(Keyword);
    void setKeywordsList(KeywordsList);

    bool projectRadioButtonEnabled();
    bool currentFileRadioButtonEnabled();
    bool buildIssuesRadioButtonEnabled();
    bool todoOutputRadioButtonEnabled();
    KeywordsList keywordsList();

signals:
    void settingsChanged();

private slots:
    void clearKeywordsList();
    void addButtonClicked();
    void removeButtonClicked();
    void resetButtonClicked();

private:
    Ui::SettingsDialog *ui;
};

#endif // SETTINGSDIALOG_H
