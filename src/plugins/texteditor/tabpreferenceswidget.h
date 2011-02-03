#ifndef TABPREFERENCESWIDGET_H
#define TABPREFERENCESWIDGET_H

#include "texteditor_global.h"

#include <QtGui/QWidget>
#include <QtCore/QMap>


namespace TextEditor {

class TabPreferences;
class IFallbackPreferences;

namespace Ui {
    class TabPreferencesWidget;
}

class TEXTEDITOR_EXPORT TabPreferencesWidget : public QWidget
{
    Q_OBJECT

public:
    explicit TabPreferencesWidget(QWidget *parent = 0);
    ~TabPreferencesWidget();

    void setTabPreferences(TabPreferences *tabPreferences);
    QString searchKeywords() const;

    void setFallbacksVisible(bool on);
    void setFlat(bool on);

protected:
    void changeEvent(QEvent *e);

private slots:
    void slotCurrentFallbackChanged(TextEditor::IFallbackPreferences *fallback);

private:
    Ui::TabPreferencesWidget *m_ui;
    TabPreferences *m_tabPreferences;
};


} // namespace TextEditor
#endif // TABPREFERENCESWIDGET_H
