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
class QToolButton;
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
    void setLabelText(const QString &text);

signals:

private slots:
    void slotComboBoxActivated(int index);
    void slotCurrentFallbackChanged(TextEditor::IFallbackPreferences *);
    void slotRestoreValues(QObject *);

private:
    IFallbackPreferences *m_fallbackPreferences;

    QHBoxLayout *m_layout;

    QComboBox *m_comboBox;
    QLabel *m_comboBoxLabel;
    QToolButton *m_restoreButton;

    bool m_fallbackWidgetVisible;
    QString m_labelText;
};

} // namespace TextEditor

#endif // FALLBACKSELECTORWIDGET_H
