#ifndef FALLBACKSELECTORWIDGET_H
#define FALLBACKSELECTORWIDGET_H

#include "texteditor_global.h"

#include <QtGui/QWidget>
#include <QtCore/QMap>

QT_BEGIN_NAMESPACE
class QHBoxLayout;
class QComboBox;
class QLabel;
class QCheckBox;
QT_END_NAMESPACE

namespace TextEditor {

class IFallbackPreferences;

class TEXTEDITOR_EXPORT FallbackSelectorWidget : public QWidget
{
    Q_OBJECT
public:
    explicit FallbackSelectorWidget(QWidget *parent = 0);

    void setFallbackPreferences(TextEditor::IFallbackPreferences *tabPreferences);
    QString searchKeywords() const;

    void setFallbacksVisible(bool on);

signals:

private slots:
    void slotComboBoxActivated(int index);
    void slotCheckBoxClicked(bool checked);
    void slotCurrentFallbackChanged(TextEditor::IFallbackPreferences *);

private:
    IFallbackPreferences *m_fallbackPreferences;

    QHBoxLayout *m_layout;

    QCheckBox *m_checkBox;
    QComboBox *m_comboBox;
    QLabel *m_comboBoxLabel;

    bool m_fallbackWidgetVisible;
    QMap<IFallbackPreferences *, int> m_fallbackToIndex;
    QMap<int, IFallbackPreferences *> m_indexToFallback;

};

} // namespace TextEditor

#endif // FALLBACKSELECTORWIDGET_H
