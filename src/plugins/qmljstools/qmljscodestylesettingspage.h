#ifndef QMLJSCODESTYLESETTINGSPAGE_H
#define QMLJSCODESTYLESETTINGSPAGE_H

#include <coreplugin/dialogs/ioptionspage.h>
#include <QtGui/QWidget>
#include <QtCore/QPointer>

QT_BEGIN_NAMESPACE
namespace Ui {
    class QmlJSCodeStyleSettingsPage;
}
class QSettings;
QT_END_NAMESPACE

namespace TextEditor {
    class FontSettings;
    class TabSettings;
    class TabPreferences;
}

namespace QmlJSTools {
namespace Internal {

class QmlJSCodeStylePreferencesWidget : public QWidget
{
    Q_OBJECT

public:
    explicit QmlJSCodeStylePreferencesWidget(QWidget *parent = 0);
    virtual ~QmlJSCodeStylePreferencesWidget();

    void setTabPreferences(TextEditor::TabPreferences *tabPreferences);

    QString searchKeywords() const;

private slots:
    void setFontSettings(const TextEditor::FontSettings &fontSettings);
    void setVisualizeWhitespace(bool on);
    void slotSettingsChanged();
    void updatePreview();

private:

    TextEditor::TabPreferences *m_tabPreferences;
    Ui::QmlJSCodeStyleSettingsPage *m_ui;
};


class QmlJSCodeStyleSettingsPage : public Core::IOptionsPage
{
    Q_OBJECT

public:
    explicit QmlJSCodeStyleSettingsPage(QWidget *parent = 0);
    ~QmlJSCodeStyleSettingsPage();

    virtual QString id() const;
    virtual QString displayName() const;
    virtual QString category() const;
    virtual QString displayCategory() const;
    virtual QIcon categoryIcon() const;

    virtual QWidget *createPage(QWidget *parent);
    virtual void apply();
    virtual void finish() { }
    virtual bool matches(const QString &) const;


private:
    QString m_searchKeywords;
    TextEditor::TabPreferences *m_pageTabPreferences;
    QPointer<QmlJSCodeStylePreferencesWidget> m_widget;
};

} // namespace Internal
} // namespace CppTools

#endif // QMLJSCODESTYLESETTINGSPAGE_H
