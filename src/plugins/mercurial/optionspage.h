#ifndef OPTIONSPAGE_H
#define OPTIONSPAGE_H

#include "ui_optionspage.h"

#include <coreplugin/dialogs/ioptionspage.h>

#include <QtGui/QWidget>
#include <QtCore/QPointer>

namespace Mercurial {
namespace Internal {

class OptionsPageWidget : public QWidget
{
    Q_OBJECT
public:
    explicit OptionsPageWidget(QWidget *parent = 0);
    void updateOptions();
    void saveOptions();

private:
    Ui::OptionsPage m_ui;
};


class OptionsPage : public Core::IOptionsPage
{
    Q_OBJECT

public:
    OptionsPage();
    QString id() const;
    QString trName() const;
    QString category() const;
    QString trCategory() const;

    QWidget *createPage(QWidget *parent);
    void apply();
    void finish() { }

signals:
    void settingsChanged();

private:
    QPointer<OptionsPageWidget> optionsPageWidget;
};

} // namespace Internal
} // namespace Mercurial

#endif // OPTIONSPAGE_H
