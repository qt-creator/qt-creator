#ifndef TEXTEDITOR_TABSETTINGSWIDGET_H
#define TEXTEDITOR_TABSETTINGSWIDGET_H

#include "texteditor_global.h"
#include <QWidget>

namespace TextEditor {

class TabSettings;

namespace Ui {
    class TabSettingsWidget;
}

class TEXTEDITOR_EXPORT TabSettingsWidget : public QWidget
{
    Q_OBJECT

public:
    explicit TabSettingsWidget(QWidget *parent = 0);
    ~TabSettingsWidget();

    TabSettings settings() const;

    void setFlat(bool on);
    QString searchKeywords() const;

public slots:
    void setSettings(const TextEditor::TabSettings& s);

signals:
    void settingsChanged(const TextEditor::TabSettings &);

protected:
    void changeEvent(QEvent *e);

private slots:
    void slotSettingsChanged();
    void updateWidget();

private:
    Ui::TabSettingsWidget *ui;
};


} // namespace TextEditor
#endif // TEXTEDITOR_TABSETTINGSWIDGET_H
