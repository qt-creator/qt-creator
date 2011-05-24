#ifndef CPPCODESTYLESETTINGSPAGE_H
#define CPPCODESTYLESETTINGSPAGE_H

#include "cpptools_global.h"

#include <coreplugin/dialogs/ioptionspage.h>
#include <QtGui/QWidget>
#include <QtCore/QPointer>
#include <QtCore/QSharedPointer>
#include <QtCore/QVariant>
#include <QtCore/QStringList>

#include "cppcodestylesettings.h"
#include "cppcodeformatter.h"

namespace TextEditor {
    class FontSettings;
    class TabSettings;
    class TabPreferences;
    class IFallbackPreferences;
    class SnippetEditorWidget;
}

namespace CppTools {

class CppCodeStylePreferences;

namespace Internal {

namespace Ui {
class CppCodeStyleSettingsPage;
}

class CppCodeStylePreferencesWidget : public QWidget
{
    Q_OBJECT
public:
    explicit CppCodeStylePreferencesWidget(QWidget *parent = 0);
    virtual ~CppCodeStylePreferencesWidget();

    void setTabPreferences(TextEditor::TabPreferences *tabPreferences);
    void setCppCodeStylePreferences(CppTools::CppCodeStylePreferences *codeStylePreferences);

    QString searchKeywords() const;

private slots:
    void setFontSettings(const TextEditor::FontSettings &fontSettings);
    void setVisualizeWhitespace(bool on);
    void slotCppCodeStyleSettingsChanged();
    void slotSettingsChanged();
    void updatePreview();
    void setCppCodeStyleSettings(const CppTools::CppCodeStyleSettings &settings);
    void slotCurrentFallbackChanged(TextEditor::IFallbackPreferences *);

signals:
    void cppCodeStyleSettingsChanged(const CppTools::CppCodeStyleSettings &);
private:
    CppCodeStyleSettings cppCodeStyleSettings() const;

    TextEditor::TabPreferences *m_tabPreferences;
    CppCodeStylePreferences *m_cppCodeStylePreferences;
    Ui::CppCodeStyleSettingsPage *m_ui;
    QList<TextEditor::SnippetEditorWidget *> m_previews;
};


class CppCodeStyleSettingsPage : public Core::IOptionsPage
{
    Q_OBJECT

public:
    explicit CppCodeStyleSettingsPage(QWidget *parent = 0);
    ~CppCodeStyleSettingsPage();

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
    CppCodeStylePreferences *m_pageCppCodeStylePreferences;
    QPointer<CppCodeStylePreferencesWidget> m_widget;
};

} // namespace Internal
} // namespace CppTools

#endif // CPPCODESTYLESETTINGSPAGE_H
